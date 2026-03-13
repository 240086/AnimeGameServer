#include "services/GachaService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "common/logger/Logger.h"
#include "game/player/PlayerManager.h"
#include "game/gacha/GachaSystem.h"
#include "network/session/SessionManager.h"

GachaService &GachaService::Instance()
{
    static GachaService instance;
    return instance;
}

void GachaService::Init()
{
    MessageDispatcher::Instance().RegisterHandler(
        MSG_GACHA,
        [this](Connection *conn, const char *data, size_t len)
        {
            HandleGacha(conn, data, len);
        });
}

void GachaService::HandleGacha(Connection *conn, const char *data, size_t len)
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
            const int COST = 160;

            if (!player->GetCurrency().Spend(COST))
            {
                LOG_WARN("not enough currency");
                return;
            }

            auto item = GachaSystem::Instance().DrawOnce(*player);

            player->GetInventory().AddItem(item.id);

            player->GetGachaHistory().Record(item.rarity);

            Packet pkt;

            pkt.SetMessageId(MSG_GACHA_DRAW);

            pkt.Append((char *)&item.id, sizeof(item.id));
            pkt.Append((char *)&item.rarity, sizeof(item.rarity));

            auto session =
                SessionManager::Instance().GetSession(sessionId);

            if (session)
            {
                auto conn = session->GetConnection();

                if (conn)
                {
                    conn->SendPacket(pkt);
                }
            }

            LOG_INFO(
                "player {} draw item {} rarity {}",
                playerId,
                item.name,
                item.rarity);
        });
}