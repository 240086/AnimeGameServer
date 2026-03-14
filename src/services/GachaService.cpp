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

    auto session =
        SessionManager::Instance().GetSession(conn->GetSessionId());

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

    /* ---------- 解析 protobuf 请求 ---------- */

    anime::GachaDrawRequest req;

    if (!req.ParseFromArray(data, (int)len))
    {
        LOG_ERROR("GachaDrawRequest parse failed");
        return;
    }

    uint64_t playerId = player->GetId();
    uint64_t sessionId = conn->GetSessionId();

    /* ---------- 投递到玩家逻辑线程 ---------- */

    player->GetCommandQueue().Push(
        [player, playerId, sessionId]()
        {
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

            /* ---------- protobuf 响应 ---------- */

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

            /* ---------- 获取连接 ---------- */

            auto session =
                SessionManager::Instance().GetSession(sessionId);

            if (!session)
                return;

            auto conn = session->GetConnection();

            if (!conn)
                return;

            conn->SendPacket(pkt);

            LOG_INFO(
                "player {} draw item {} rarity {}",
                playerId,
                item.name,
                item.rarity);
        });
}

void GachaService::HandleGachaTen(Connection* conn, const char* data, size_t len)
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

    player->GetCommandQueue().Push(
        [player, playerId, sessionId]()
        {
            const int COST = 1600;

            if (!player->GetCurrency().Spend(COST))
            {
                LOG_WARN("not enough currency");
                return;
            }

            for (int i = 0; i < 10; ++i)
            {
                auto item = GachaSystem::Instance().DrawOnce(*player);

                player->GetInventory().AddItem(item.id);

                player->GetGachaHistory().Record(item.rarity);

                anime::GachaDrawResponse resp;

                resp.set_item_id(item.id);
                resp.set_rarity(item.rarity);

                Packet pkt;
                pkt.SetMessageId(MSG_S2C_GACHA_DRAW_RESP);

                auto bytes = resp.SerializeAsString();
                pkt.Append(bytes.data(), bytes.size());

                auto session =
                    SessionManager::Instance().GetSession(sessionId);

                if (session)
                {
                    auto conn = session->GetConnection();

                    if (conn)
                        conn->SendPacket(pkt);
                }
            }

            LOG_INFO("player {} ten draw finished", playerId);
        });
}