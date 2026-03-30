#include "services/LoginService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/ProtoMessage.h"
#include "network/session/SessionManager.h"
#include "game/player/PlayerManager.h"
#include "common/logger/Logger.h"
#include "common/thread/GlobalThreadPool.h"
#include "database/player/PlayerLoader.h"
#include "database/repository/AccountRepository.h"
#include "network/protocol/ErrorSender.h"
#include "network/protocol/ResponseSender.h"
#include "login.pb.h"

LoginService &LoginService::Instance()
{
    static LoginService instance;
    return instance;
}

void LoginService::Init()
{
    // 注册登录消息处理器
    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_LOGIN,
        [this](const MessageContext &ctx, std::shared_ptr<anime::IMessage> msg)
        {
            // 使用 move 将消息所有权转移至异步处理流程，减少引用计数操作
            this->HandleLogin(ctx, std::move(msg));
        });
}

void LoginService::HandleLogin(const MessageContext &ctx, std::shared_ptr<anime::IMessage> msg)
{
    // 基础合法性检查
    if (!ctx.conn || !msg)
        return;

    LOG_DEBUG("ENTERING HandleLogin: ctx_addr={}, sid={}, raw_sid_hex=0x{:08X}",
              (void *)&ctx, ctx.sid, ctx.sid);

    // 1. 尝试将基类 IMessage 转换为具体的 ProtoMessage 包装器
    auto loginMsg = std::dynamic_pointer_cast<ProtoMessage<anime::LoginRequest>>(msg);
    if (!loginMsg)
    {
        LOG_ERROR("Login parse failed, sid={}", ctx.sid);
        ErrorSender::Send(ctx, ErrorCode::INVALID_REQUEST);
        return;
    }

    // 2. 提前提取数据，ctx.conn 放入捕获列表确保异步期间连接对象不被析构
    auto connPtr = ctx.conn;
    auto username = loginMsg->Get().username();
    auto password = loginMsg->Get().password();
    uint32_t sid = ctx.sid;
    uint32_t seqId = ctx.seqId;
    auto protoType = ctx.protoType;

    // 3. 投递到全局线程池执行 DB 负载（避免阻塞网关 IO 线程）
    // 💡 必须捕获 ctx 以便在回调中使用 sid 和 seqId 寻址
    GlobalThreadPool::Instance().GetPool().Enqueue(
        [sid, seqId, protoType, connPtr, username = std::move(username), password = std::move(password)]() mutable
        {
            MessageContext ctx;
            ctx.sid = sid;
            ctx.seqId = seqId;
            ctx.protoType = protoType;
            ctx.conn = connPtr;
            uint64_t playerId = 0;

#ifdef STRESS_TEST_MODE
            // 压测模式：直接转换 ID，无需校验数据库
            try
            {
                playerId = std::stoull(username);
            }
            catch (...)
            {
                ErrorSender::Send(ctx, ErrorCode::AUTH_FAILED);
                return;
            }
#else
            // 正常模式：校验账号密码
            auto playerIdOpt = AccountRepository::Instance().GetAccountId(username, password);
            if (!playerIdOpt)
            {
                // 注意：DB 线程执行完必须通过 asio::post 切回该连接所属的 executor 执行发送
                boost::asio::post(connPtr->GetSocket().get_executor(),
                                  [ctx]()
                                  { 
                                    auto session = SessionManager::Instance().GetSession(ctx.sid);
                                    if (!session) return;
                                    ErrorSender::Send(ctx, ErrorCode::AUTH_FAILED); });
                return;
            }
            playerId = *playerIdOpt;
#endif

            // 4. 加载玩家数据对象
            auto player = std::make_shared<Player>(playerId);
            if (!PlayerLoader::Load(playerId, *player))
            {
                LOG_ERROR("Load player failed: {}", playerId);
                boost::asio::post(connPtr->GetSocket().get_executor(),
                                  [ctx]()
                                  { auto session = SessionManager::Instance().GetSession(ctx.sid);
                                    if (!session) return;
                                    ErrorSender::Send(ctx, ErrorCode::PLAYER_LOAD_FAILED); });
                return;
            }

            // 5. 核心逻辑回流：切回网络线程，执行 Session 绑定与 Actor 激活
            boost::asio::post(connPtr->GetSocket().get_executor(),
                              [ctx, connPtr, playerId, player]()
                              {
                                  // 获取或创建针对该网关 sid 的 Session
                                  auto session = SessionManager::Instance().GetSession(ctx.sid);
                                  if (!session)
                                  {
                                      // 第一次登录请求时，后端可能尚未为此 sid 创建 Session
                                      session = SessionManager::Instance().CreateSessionWithId(ctx.sid);
                                      session->BindConnection(connPtr);
                                  }

                                  // 6. 执行顶号逻辑与内存状态同步
                                  // 确保全服只有一个 Session 拥有该 playerId
                                  SessionManager::Instance().KickPlayer(playerId, ctx.sid, "Re-login");
                                  PlayerManager::Instance().Logout(playerId);

                                  if (!PlayerManager::Instance().Login(playerId, player))
                                  {
                                      ErrorSender::Send(ctx, ErrorCode::AUTH_FAILED);
                                      return;
                                  }

                                  // 7. 绑定 Actor 系统，将该玩家消息序列化
                                  auto actor = std::make_shared<PlayerActor>(player);
                                  session->BindPlayer(player);
                                  session->BindActor(actor);
                                  player->SetSessionId(ctx.sid);

                                  // 在管理器中维护索引，方便后续通过 playerId 找会话
                                  SessionManager::Instance().BindPlayerToSession(playerId, session);

                                  // 8. 构造并发送登录成功响应
                                  anime::LoginResponse resp;
                                  resp.set_player_id(playerId);
                                  resp.set_currency(player->GetCurrency().Get());

                                  // ResponseSender 会自动利用 ctx.sid 和 ctx.seqId 将包正确发回网关
                                  ResponseSender::Send(ctx, MSG_S2C_LOGIN_RESP, resp);

                                  LOG_INFO("Player {} login success, sid={}, seqId={}",
                                           playerId, ctx.sid, ctx.seqId);
                              });
        });
}