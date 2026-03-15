#include "services/LoginService.h"

#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/Packet.h"

#include "network/session/SessionManager.h"
#include "game/player/PlayerManager.h"

#include "common/logger/Logger.h"

#include "network/protocol/generated/login.pb.h"
#include "database/player/PlayerLoader.h"

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
    LOG_INFO("login request received size={}", len);

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

    // 3. 确定玩家 ID
    // 专业提示：实际项目中这里应该是根据 request.token() 或 username 从数据库/Auth服务查询 UID
    static std::atomic<uint64_t> next_player_id{1};
    uint64_t playerId = next_player_id++;

    /* ---------- 专业逻辑开始：顶号检查 (Kick Old Connection) ---------- */
    // 如果该 UID 已经在线，需要先踢掉旧的连接
    auto oldPlayer = PlayerManager::Instance().GetPlayer(playerId);
    if (oldPlayer)
    {
        LOG_WARN("Player {} already login, kicking old one...", playerId);
        // 在实际架构中，这里会触发 oldPlayer->GetSession()->GetConnection()->Close()
    }

    /* ---------- 修复点：手动创建并添加玩家 ---------- */
    // 4. 创建玩家对象 (不在 Manager 内部创建)
    auto player = std::make_shared<Player>(playerId);

    // 从数据库加载
    if (!PlayerLoader::Load(playerId, *player))
    {
        LOG_ERROR("Load player failed {}", playerId);
        return;
    }

    // 5. 创建对应的 Actor 执行体
    auto actor = std::make_shared<PlayerActor>(player);

    // 6. 将玩家加入全局管理器（此时玩家正式“在线”）
    PlayerManager::Instance().AddPlayer(player);

    // 7. 绑定 Session 引用链
    session->BindPlayer(player);
    session->BindActor(actor);

    /* ---------- 逻辑 Actor 化：所有状态修改必须在 Post 内 ---------- */
    // 8. 投递初始化任务

    auto weakConn = conn->weak_from_this();
    actor->Post([weakConn, player, playerId]()
                {
        // 重要：初次登录的奖励、属性计算等逻辑应在 Actor 线程执行，确保线程安全
        player->GetCurrency().Set(1000000000); 

        auto connPtr = weakConn.lock();
        if (!connPtr)
        {
            LOG_WARN("Login response aborted: Connection lost for player {}", playerId);
            return; 
        }

        anime::LoginResponse resp_pb;
        resp_pb.set_player_id(playerId);
        resp_pb.set_currency(player->GetCurrency().Get());

        std::string payload;
        if (!resp_pb.SerializeToString(&payload)) {
            LOG_ERROR("LoginResponse serialize failed");
            return;
        }

        Packet packet;
        packet.SetMessageId(MSG_S2C_LOGIN_RESP);
        packet.Append(payload.data(), payload.size());

        // 发送响应
        connPtr->SendPacket(packet);

        LOG_INFO("Player {} login success. Currency: {}", 
                 playerId, player->GetCurrency().Get()); });
}