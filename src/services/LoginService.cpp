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
    if (!ctx.conn || !msg)
        return;

    auto loginMsg = std::dynamic_pointer_cast<ProtoMessage<anime::LoginRequest>>(msg);
    if (!loginMsg)
    {
        LOG_ERROR("Login parse failed, sid={}", ctx.sid);
        ErrorSender::Send(ctx, ErrorCode::INVALID_REQUEST);
        return;
    }

    // 提取闭包所需数据
    auto connPtr = ctx.conn;
    auto username = loginMsg->Get().username();
    auto password = loginMsg->Get().password();
    uint32_t sid = ctx.sid;
    uint32_t seqId = ctx.seqId;
    auto protoType = ctx.protoType;

    // 投递到 DB 线程池
    GlobalThreadPool::Instance().GetPool().Enqueue(
        [sid, seqId, protoType, connPtr, username = std::move(username), password = std::move(password)]() mutable
        {
            // 重新构造 DB 阶段的 Context
            MessageContext dbCtx;
            dbCtx.sid = sid;
            dbCtx.seqId = seqId;
            dbCtx.protoType = protoType;
            dbCtx.conn = connPtr;

            uint64_t playerId = 0;

#ifdef STRESS_TEST_MODE
            try
            {
                playerId = std::stoull(username);
            }
            catch (...)
            {
                ErrorSender::Send(dbCtx, ErrorCode::AUTH_FAILED);
                return;
            }
#else
            auto playerIdOpt = AccountRepository::Instance().GetAccountId(username, password);
            if (!playerIdOpt)
            {
                boost::asio::post(connPtr->GetSocket().get_executor(),
                                  [dbCtx]()
                                  { ErrorSender::Send(dbCtx, ErrorCode::AUTH_FAILED); });
                return;
            }
            playerId = *playerIdOpt;
#endif

            // 加载玩家数据
            auto player = std::make_shared<Player>(playerId);
            if (!PlayerLoader::Load(playerId, *player))
            {
                LOG_ERROR("Load player failed: {}", playerId);
                boost::asio::post(connPtr->GetSocket().get_executor(),
                                  [dbCtx]()
                                  { ErrorSender::Send(dbCtx, ErrorCode::PLAYER_LOAD_FAILED); });
                return;
            }

            // 核心逻辑回流：切回网络/逻辑线程执行绑定
            boost::asio::post(connPtr->GetSocket().get_executor(),
                              [dbCtx, connPtr, playerId, player]() mutable
                              {
                                  // 1. 获取并激活 Session
                                  auto session = SessionManager::Instance().GetSession(dbCtx.sid);
                                  if (!session)
                                  {
                                      session = SessionManager::Instance().CreateSessionWithId(dbCtx.sid);
                                      session->BindConnection(connPtr);
                                  }

                                  // 2. 顶号逻辑 (互斥处理)
                                  // 必须先在 SessionManager 层面踢出，防止多处 sid 指向同一个 uid
                                  SessionManager::Instance().KickPlayer(playerId, dbCtx.sid, "Re-login");
                                  // 确保内存中旧的 player 实例被正确清理和存档
                                  PlayerManager::Instance().Logout(playerId);

                                  // 3. 将新玩家对象注入管理器
                                  if (!PlayerManager::Instance().Login(playerId, player))
                                  {
                                      ErrorSender::Send(dbCtx, ErrorCode::AUTH_FAILED);
                                      return;
                                  }

                                  // 4. 关键：建立 Session, Player, Actor 的三方绑定
                                  // 绑定到 Session 内部，这样 main.cpp 的 onPacket 才能在后续包里找到 ctx.player
                                  session->BindPlayer(player);

                                  auto actor = std::make_shared<PlayerActor>(player);
                                  session->BindActor(actor);

                                  // 建立 UID -> Session 的反向索引
                                  SessionManager::Instance().BindPlayerToSession(playerId, session);

                                  // 5. 更新 Context 的灵魂：填充 player
                                  dbCtx.player = player;
                                  dbCtx.session = session;
                                  player->SetSessionId(dbCtx.sid);

                                  // 6. 构造响应
                                  anime::LoginResponse resp;
                                  resp.set_player_id(playerId);
                                  resp.set_currency(player->GetCurrency().Get());

                                  // 发送响应（此时 dbCtx.player 已经有值，ResponseSender 会非常安全）
                                  ResponseSender::Send(dbCtx, MSG_S2C_LOGIN_RESP, resp);

                                  LOG_DEBUG("Player {} login success, sid={}, seqId={}",
                                           playerId, dbCtx.sid, dbCtx.seqId);
                              });
        });
}