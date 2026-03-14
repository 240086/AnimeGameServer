#include "services/GachaService.h"

#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/Packet.h"

#include "network/session/SessionManager.h"

#include "game/gacha/GachaSystem.h"

#include "common/logger/Logger.h"

#include "network/protocol/generated/gacha.pb.h"
#include "game/player/Player.h"

GachaService &GachaService::Instance()
{
    static GachaService instance;
    return instance;
}

void GachaService::Init()
{
    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_GACHA_DRAW,
        [this](Connection *conn, const char *data, size_t len)
        {
            HandleGacha(conn, data, len);
        });

    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_GACHA_DRAW_TEN,
        [this](Connection *conn, const char *data, size_t len)
        {
            HandleGachaTen(conn, data, len);
        });
}

void GachaService::HandleGacha(Connection *conn, const char *data, size_t len)
{
    if (!conn)
        return;

    // 1. 获取 Session（IO线程安全访问）
    auto session = SessionManager::Instance().GetSession(conn->GetSessionId());
    if (!session)
    {
        LOG_ERROR("session not found for sessionId: {}", conn->GetSessionId());
        return;
    }

    // 2. 获取 Actor（Session 必须已经绑定了 PlayerActor）
    auto actor = session->GetActor();
    auto player = session->GetPlayer();

    if (!actor || !player)
    {
        LOG_ERROR("player not login or actor not bound");
        return;
    }

    // 3. 解析 Protobuf（在 IO 线程解析，减轻逻辑线程负担）
    anime::GachaDrawRequest req;
    if (!req.ParseFromArray(data, (int)len))
    {
        LOG_ERROR("GachaDrawRequest parse failed");
        return;
    }

    uint64_t playerId = player->GetId();
    uint64_t sessionId = conn->GetSessionId();

    // 4. 核心：通过 PlayerActor 投递任务
    // 捕获 actor (shared_ptr) 以确保在逻辑执行期间 Player 和 Actor 对象不会析构
    actor->Post(
        [actor, playerId, sessionId]()
        {
            // --- 以下代码在 ActorSystem Worker 线程执行，绝对单线程安全 ---
            auto player = actor->GetPlayer();

            // 防御性检查：确保绑定的 Player 实例依然有效
            if (!player)
                return;

            const int COST = 160;

            // 扣费逻辑
            if (!player->GetCurrency().Spend(COST))
            {
                LOG_WARN("player {} not enough currency", playerId);
                // 实际项目中这里应该发一个错误码响应给客户端
                return;
            }

            // 抽卡核心逻辑（GachaSystem 内部需保证只读或线程安全）
            auto item = GachaSystem::Instance().DrawOnce(*player);

            // 写入玩家数据（无需加锁，因为处于 PlayerActor 串行上下文中）
            player->GetInventory().AddItem(item.id);
            player->GetGachaHistory().Record(item.rarity);

            // 构造响应包
            anime::GachaDrawResponse resp_pb;
            resp_pb.set_item_id(item.id);
            resp_pb.set_rarity(item.rarity);

            std::string payload;
            if (!resp_pb.SerializeToString(&payload))
            {
                LOG_ERROR("GachaDrawResponse serialize failed");
                return;
            }

            Packet pkt;
            pkt.SetMessageId(MSG_S2C_GACHA_DRAW_RESP);
            pkt.Append(payload.data(), payload.size());

            // 5. 回路投递：重新获取 Session 发送结果
            // 注意：不要在 Lambda 中直接捕获之前的 Connection 指针，因为 Connection 可能已断开
            auto currentSession = SessionManager::Instance().GetSession(sessionId);
            if (!currentSession)
            {
                LOG_DEBUG("session {} closed during gacha process", sessionId);
                return;
            }

            auto currentConn = currentSession->GetConnection();
            if (currentConn)
            {
                currentConn->SendPacket(pkt);
            }

            LOG_INFO("player {} draw item_id {} rarity {} success",
                     playerId, item.id, item.rarity);
        });
}

void GachaService::HandleGachaTen(Connection *conn, const char *data, size_t len)
{
    if (!conn)
        return;

    auto session = SessionManager::Instance().GetSession(conn->GetSessionId());

    if (!session)
    {
        LOG_ERROR("session not found");
        return;
    }

    auto player = session->GetPlayer();

    if (!player)
    {
        LOG_ERROR("player not login");
        return;
    }

    uint64_t playerId = player->GetId();
    uint64_t sessionId = conn->GetSessionId();

    auto actor = session->GetActor();

    actor->Post(
        [actor, playerId, sessionId]()
        {
            auto player = actor->GetPlayer();

            const int COST = 160;

            if (!player->GetCurrency().Spend(COST))
            {
                LOG_WARN("player {} not enough currency", playerId);
                return;
            }

            auto item =
                GachaSystem::Instance().DrawOnce(*player);

            player->GetInventory().AddItem(item.id);

            player->GetGachaHistory().Record(item.rarity);

            anime::GachaDrawResponse resp_pb;

            resp_pb.set_item_id(item.id);
            resp_pb.set_rarity(item.rarity);

            std::string payload;

            if (!resp_pb.SerializeToString(&payload))
            {
                LOG_ERROR("serialize failed");
                return;
            }

            Packet pkt;

            pkt.SetMessageId(MSG_S2C_GACHA_DRAW_RESP);

            pkt.Append(payload.data(), payload.size());

            auto session =
                SessionManager::Instance().GetSession(sessionId);

            if (!session)
                return;

            auto conn = session->GetConnection();

            if (conn)
                conn->SendPacket(pkt);
        });
}