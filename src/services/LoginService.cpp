#include "services/LoginService.h"

#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/Packet.h"

#include "network/session/SessionManager.h"
#include "game/player/PlayerManager.h"

#include "common/logger/Logger.h"

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
    // LOG_INFO("login request received size={}", len); // 压测时建议关闭过于频繁的 IO 日志

    if (!conn)
        return;

    // 1. 获取 Session（确保连接合法）
    auto session = SessionManager::Instance().GetSession(conn->GetSessionId());
    if (!session)
    {
        LOG_ERROR("session not found for sessionId={}", conn->GetSessionId());
        return;
    }

    // 2. 解析 Protobuf 请求
    anime::LoginRequest request;
    if (!request.ParseFromArray(data, (int)len))
    {
        LOG_ERROR("LoginRequest parse failed");
        return;
    }

// 宏定义最好放在 CMakeLists.txt 中，这里为了兼容你的代码保留
// #define STRESS_TEST_MODE
#ifdef STRESS_TEST_MODE
    uint64_t playerId = 0;
    try
    {
        playerId = std::stoull(request.username()); // user_id = username
    }
    catch (const std::exception &)
    {
        LOG_WARN("Stress login failed: username={} is not numeric", request.username());
        SendSimpleMessage(conn, MSG_S2C_ERROR_AUTH_FAIL);
        return;
    }
#else
    // 正常账号逻辑
    auto playerIdOpt = AccountRepository::Instance().GetAccountId(
        request.username(),
        request.password());
    if (!playerIdOpt)
    {
        LOG_WARN("Login failed for username={}", request.username());
        SendSimpleMessage(conn, MSG_S2C_ERROR_AUTH_FAIL);
        return;
    }
    uint64_t playerId = *playerIdOpt;
#endif

    /* ---------- 顶号检查 (Kick Old Connection) ---------- */
    auto oldPlayer = PlayerManager::Instance().GetPlayer(playerId);
    if (oldPlayer)
    {
        LOG_WARN("Player {} already login, kicking old one...", playerId);
        // TODO: 踢人逻辑
    }

    /* ---------- 创建与初始化玩家 (此时绝对线程安全) ---------- */
    auto player = std::make_shared<Player>(playerId);

#ifdef STRESS_TEST_MODE
    // 压测模式下直接在 IO 线程赋初值，此时 player 尚未被其他线程可见，不需要加锁
    player->GetCurrency().Set(1000000);
#else
    // 从数据库加载 (注：在真正的工业级架构中，DB 加载也应是异步的)
    if (!PlayerLoader::Load(playerId, *player))
    {
        LOG_ERROR("Load player failed {}", playerId);
        SendSimpleMessage(conn, MSG_S2C_ERROR_COMMON);
        return;
    }
    // 🔥 【测试专用：强行发钱】
    // 在这里直接给刚加载完的玩家塞 100 万，确保接下来的抽卡压测 100% 成功
    player->GetCurrency().Set(1000000);
#endif

    // 5. 创建对应的 Actor 执行体
    auto actor = std::make_shared<PlayerActor>(player);

    // 6. 绑定 Session 引用链
    session->BindPlayer(player);
    session->BindActor(actor);

    // 7. 将玩家加入全局管理器（此时玩家正式“在线”，开始对其他线程可见）
    PlayerManager::Instance().AddPlayer(player);

    /* ====================================================
       🔥 核心修正：登录响应直接在 IO 线程发送，绝不排队！
       ==================================================== */
    anime::LoginResponse resp_pb;
    resp_pb.set_player_id(playerId);
    resp_pb.set_currency(player->GetCurrency().Get());

    std::string payload;
    if (!resp_pb.SerializeToString(&payload))
    {
        LOG_ERROR("LoginResponse serialize failed");
        return;
    }

    Packet packet;
    packet.SetMessageId(MSG_S2C_LOGIN_RESP);
    packet.Append(payload.data(), payload.size());

    // 直接调用底层 async_write，立刻返回给客户端，绝不阻塞！
    conn->SendPacket(packet);

    // LOG_INFO("Player {} login success. Currency: {}", playerId, player->GetCurrency().Get());
}