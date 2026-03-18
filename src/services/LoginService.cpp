#include "services/LoginService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/Packet.h"
#include "network/session/SessionManager.h"
#include "game/player/PlayerManager.h"
#include "common/logger/Logger.h"
#include "common/thread/GlobalThreadPool.h"
#include "network/protocol/generated/login.pb.h"
#include "database/player/PlayerLoader.h"
#include "database/repository/AccountRepository.h"

namespace
{
    void SendSimpleMessage(Connection *conn, uint16_t messageId)
    {
        if (!conn)
            return;
        Packet packet;
        packet.SetMessageId(messageId);
        conn->SendPacket(packet);
    }
}

LoginService &LoginService::Instance()
{
    static LoginService instance;
    return instance;
}

void LoginService::Init()
{
    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_LOGIN,
        [this](Connection *conn, const char *data, size_t len)
        {
            HandleLogin(conn, data, len);
        });
}

void LoginService::HandleLogin(Connection *conn, const char *data, size_t len)
{
    if (!conn)
        return;

    // 1. 捕获共享指针，确保异步期间对象存活
    auto connPtr = conn->shared_from_this();

    // 2. 解析 Protobuf 请求（快速完成）
    anime::LoginRequest request;
    if (!request.ParseFromArray(data, (int)len))
    {
        LOG_ERROR("LoginRequest parse failed");
        return;
    }

    auto username = request.username();
    auto password = request.password();

    // 3. 将耗时操作（DB/计算）投递到全局线程池
    GlobalThreadPool::Instance().GetPool().Enqueue(
        [connPtr, username = std::move(username), password = std::move(password)]()
        {
            uint64_t playerId = 0;

#ifdef STRESS_TEST_MODE
            try
            {
                // 压测模式：直接将用户名视为 UID
                playerId = std::stoull(username);
            }
            catch (const std::exception &)
            {
                boost::asio::post(connPtr->GetSocket().get_executor(),
                                  [connPtr]()
                                  { SendSimpleMessage(connPtr.get(), MSG_S2C_ERROR_AUTH_FAIL); });
                return;
            }
#else
            // 正常逻辑：查询数据库账号
            auto playerIdOpt = AccountRepository::Instance().GetAccountId(username, password);
            if (!playerIdOpt)
            {
                boost::asio::post(connPtr->GetSocket().get_executor(),
                                  [connPtr]()
                                  { SendSimpleMessage(connPtr.get(), MSG_S2C_ERROR_AUTH_FAIL); });
                return;
            }
            playerId = *playerIdOpt;
#endif

            // 4. 加载玩家数据（耗时点）
            auto player = std::make_shared<Player>(playerId);

#ifndef STRESS_TEST_MODE
            if (!PlayerLoader::Load(playerId, *player))
            {
                LOG_ERROR("Load player failed: {}", playerId);
                boost::asio::post(connPtr->GetSocket().get_executor(),
                                  [connPtr]()
                                  { SendSimpleMessage(connPtr.get(), MSG_S2C_ERROR_COMMON); });
                return;
            }
#endif
            // 【压测/测试逻辑】强行赋予足够余额
            player->GetCurrency().Set(1000000);

            // 5. 回流至该连接所属的 IO 线程进行最终状态变更和回包
            boost::asio::post(
                connPtr->GetSocket().get_executor(),
                [connPtr, playerId, player]()
                {
                    uint64_t sid = connPtr->GetSessionId();
                    std::shared_ptr<Session> session;

                    // 绑定或创建 Session
                    if (sid == 0)
                    {
                        session = SessionManager::Instance().CreateSession();
                        session->BindConnection(connPtr);
                        connPtr->SetSessionId(session->GetSessionId());
                    }
                    else
                    {
                        session = SessionManager::Instance().GetSession(sid);
                        if (!session)
                        {
                            LOG_ERROR("Session not found for sid={} during login", sid);
                            return;
                        }
                    }

                    // 顶号逻辑占位
                    auto oldPlayer = PlayerManager::Instance().GetPlayer(playerId);
                    if (oldPlayer)
                    {
                        LOG_WARN("Player {} already login, kicking old one...", playerId);
                        // TODO: Implement Kick Logic
                    }

                    // 正式上线
                    auto actor = std::make_shared<PlayerActor>(player);
                    session->BindPlayer(player);
                    session->BindActor(actor);
                    PlayerManager::Instance().AddPlayer(player);

                    // 构建并发送响应包
                    anime::LoginResponse resp_pb;
                    resp_pb.set_player_id(playerId);
                    resp_pb.set_currency(player->GetCurrency().Get());

                    std::string payload;
                    if (resp_pb.SerializeToString(&payload))
                    {
                        Packet packet;
                        packet.SetMessageId(MSG_S2C_LOGIN_RESP);
                        packet.Append(payload.data(), payload.size());
                        connPtr->SendPacket(packet);
                    }

                    LOG_INFO("Player {} login success via Async-Path, sid={}", playerId, sid);
                });
        });
}