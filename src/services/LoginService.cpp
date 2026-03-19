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
#include "network/protocol/messages/LoginMessage.h"

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
        [this](Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
        {
            // 注意：登录时 player 为 nullptr
            HandleLogin(conn, msg);
        });
}

void LoginService::HandleLogin(Connection *conn, std::shared_ptr<IMessage> msg)
{
    if (!conn || !msg)
        return;

    // 1. 捕获共享指针，确保异步期间对象存活
    auto connPtr = conn->shared_from_this();

    // // 2. 解析 Protobuf 请求（快速完成）
    // anime::LoginRequest request;
    // if (!request.ParseFromArray(data, (int)len))
    // {
    //     LOG_ERROR("LoginRequest parse failed");
    //     return;
    // }
    auto loginMsg = std::dynamic_pointer_cast<LoginMessage>(msg);
    if (!loginMsg)
    {
        LOG_ERROR("Message type mismatch: expected LoginMessage");
        return;
    }

    // 直接从解析好的 loginMsg 中提取数据，不再需要 ParseFromArray
    auto username = loginMsg->username;
    auto password = loginMsg->password;

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

                    // --- Session 获取或创建 ---
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

                    // --- 🔥 顶号与唯一性控制 (工业级闭环) ---

                    // 1️⃣ 断开旧连接：传入当前 sid 以排除自己，避免“自杀式踢人”
                    SessionManager::Instance().KickPlayer(playerId, sid, "Re-login");

                    // 2️⃣ 剥夺旧 Player 写权限：同步调用 Logout，利用内部 CAS 标记 is_logging_out_
                    // 即使旧连接的物理断开回调还没触发，这里的 Logout 也会立即让旧实例失效并触发保存
                    PlayerManager::Instance().Logout(playerId);

                    // 2. 正式登录（进入 PlayerManager 内存容器）
                    // 此时可以安全地放入新 player 实例了
                    if (!PlayerManager::Instance().Login(playerId, player))
                    {
                        LOG_ERROR("Player {} Login to PlayerManager failed - potential race condition", playerId);
                        SendSimpleMessage(connPtr.get(), MSG_S2C_ERROR_COMMON);
                        return;
                    }

                    // 3. 绑定组件与映射
                    auto actor = std::make_shared<PlayerActor>(player);
                    session->BindPlayer(player);
                    session->BindActor(actor);
                    player->SetSessionId(session->GetSessionId());

                    // 🔥 建立 PlayerId -> Session 的映射，供后续 KickPlayer 查询
                    SessionManager::Instance().BindPlayerToSession(playerId, session);

                    // --- 构建回包 ---
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

                    LOG_INFO("Player {} login success, sid={}, is_relogin={}",
                             playerId, sid, (sid != 0));
                });
        });
}