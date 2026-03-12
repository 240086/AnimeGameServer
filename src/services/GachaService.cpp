#include "services/GachaService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "common/logger/Logger.h"
#include "game/player/PlayerManager.h"
#include "game/gacha/GachaSystem.h"

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

    uint64_t playerId = conn->GetPlayerId();

    auto player = PlayerManager::Instance().GetPlayer(playerId);

    if (!player)
    {
        LOG_ERROR("player not found {}", playerId);
        return;
    }

    const int COST = 160;
    std::lock_guard<std::mutex> lock(player->GetMutex());

    if (!player->GetCurrency().Spend(COST))
    {
        LOG_WARN("not enough currency");
        return;
    }

    player->GetCommandQueue().Push(
        [player, playerId]()
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

            LOG_INFO(
                "player {} draw item {} rarity {}",
                playerId,
                item.name,
                item.rarity);
        });
}