#include "services/LoginService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/Packet.h"
#include "network/protocol/ProtoMessage.h"
#include "network/session/SessionManager.h"
#include "game/player/PlayerManager.h"
#include "common/logger/Logger.h"
#include "common/thread/GlobalThreadPool.h"
#include "database/player/PlayerLoader.h"
#include "database/repository/AccountRepository.h"
#include "network/protocol/ErrorSender.h"
#include "login.pb.h"

LoginService &LoginService::Instance()
{
    static LoginService instance;
    return instance;
}

void LoginService::Init()
{
    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_LOGIN,
        [this](Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
        {
            // 💡 登录时 player 必定为 nullptr，显式忽略以消除警告
            (void)player;
            // 使用 move 将所有权转移至异步处理流程
            HandleLogin(conn, std::move(msg));
        });
}

void LoginService::HandleLogin(Connection *conn, std::shared_ptr<IMessage> msg)
{
    if (!conn || !msg)
        return;

    // 🔥 核心变更：统一使用 ProtoMessage 泛型包装器
    auto loginMsg = std::dynamic_pointer_cast<ProtoMessage<anime::LoginRequest>>(msg);
    if (!loginMsg)
    {
        LOG_ERROR("Login parse failed");
        ErrorSender::Send(conn, ErrorCode::INVALID_REQUEST);
        return;
    }

    // 捕获 shared_ptr 保证异步期间 Connection 实例不被销毁
    auto connPtr = conn->shared_from_this();

    // 从 PB 对象提取数据
    auto username = loginMsg->Get().username();
    auto password = loginMsg->Get().password();

    // 投递到全局线程池处理 DB 负载（避免阻塞 IO 线程）
    GlobalThreadPool::Instance().GetPool().Enqueue(
        [connPtr, username = std::move(username), password = std::move(password)]()
        {
            uint64_t playerId = 0;

#ifdef STRESS_TEST_MODE
            try
            {
                playerId = std::stoull(username);
            }
            catch (const std::exception &)
            {
                boost::asio::post(connPtr->GetSocket().get_executor(),
                                  [connPtr]()
                                  { ErrorSender::Send(connPtr.get(), ErrorCode::AUTH_FAILED); });
                return;
            }
#else
            auto playerIdOpt = AccountRepository::Instance().GetAccountId(username, password);
            if (!playerIdOpt)
            {
                boost::asio::post(connPtr->GetSocket().get_executor(),
                                  [connPtr]()
                                  { ErrorSender::Send(connPtr.get(), ErrorCode::AUTH_FAILED); });
                return;
            }
            playerId = *playerIdOpt;
#endif

            auto player = std::make_shared<Player>(playerId);

#ifndef STRESS_TEST_MODE
            if (!PlayerLoader::Load(playerId, *player))
            {
                LOG_ERROR("Load player failed: {}", playerId);
                boost::asio::post(connPtr->GetSocket().get_executor(),
                                  [connPtr]()
                                  { ErrorSender::Send(connPtr.get(), ErrorCode::PLAYER_LOAD_FAILED); });
                return;
            }
#endif
            // 测试环境下赋予足够货币
            player->GetCurrency().Set(1000000);

            // 关键：将状态变更逻辑回流 (Post) 到连接所属的 IO 线程，保证单线程安全执行
            boost::asio::post(
                connPtr->GetSocket().get_executor(),
                [connPtr, playerId, player]()
                {
                    uint64_t sid = connPtr->GetSessionId();
                    std::shared_ptr<Session> session;

                    if (sid == 0)
                    {
                        session = SessionManager::Instance().CreateSession();
                        session->BindConnection(connPtr);
                        sid = session->GetSessionId();
                        connPtr->SetSessionId(sid);
                    }
                    else
                    {
                        session = SessionManager::Instance().GetSession(sid);
                        if (!session)
                            return;
                    }

                    // 顶号逻辑：确保同一账号在全服只有一处在线
                    SessionManager::Instance().KickPlayer(playerId, sid, "Re-login");
                    PlayerManager::Instance().Logout(playerId);

                    if (!PlayerManager::Instance().Login(playerId, player))
                    {
                        LOG_ERROR("Player {} login to PlayerManager failed", playerId);
                        ErrorSender::Send(connPtr.get(), ErrorCode::AUTH_FAILED);
                        return;
                    }

                    // 绑定各组件
                    auto actor = std::make_shared<PlayerActor>(player);
                    session->BindPlayer(player);
                    session->BindActor(actor);
                    player->SetSessionId(session->GetSessionId());

                    SessionManager::Instance().BindPlayerToSession(playerId, session);

                    // 构造响应回包
                    anime::LoginResponse respPb;
                    respPb.set_player_id(playerId);
                    respPb.set_currency(player->GetCurrency().Get());

                    std::string payload;
                    if (respPb.SerializeToString(&payload))
                    {
                        Packet packet;
                        packet.SetMessageId(MSG_S2C_LOGIN_RESP);
                        packet.Append(payload); // 使用更简洁的 Append(std::string)
                        connPtr->SendPacket(packet);
                    }

                    LOG_INFO("Player {} login success, sid={}", playerId, sid);
                });
        });
}