#include "game/gacha/GachaSystem.h"
#include "common/logger/Logger.h"
#include "game/gacha/PitySystem.h"
#include "game/player/Player.h"
#include "game/gacha/GachaPoolManager.h"

GachaSystem &GachaSystem::Instance()
{
    static GachaSystem instance;
    return instance;
}

GachaItem GachaSystem::DrawOnce(Player &player, int poolId)
{
    auto &history = player.GetGachaHistory();

    int rarity = PitySystem::RollRarity(history);

    // 注意：只在这里记录一次，避免服务层重复 Record 导致保底计数失真
    history.Record(rarity);

    int targetPoolId = (poolId <= 0) ? default_pool_id_ : poolId;
    auto &pool = GachaPoolManager::Instance().GetPool(targetPoolId);
    return pool.DrawByRarity(rarity);
}

std::vector<GachaItem> GachaSystem::DrawTen(Player &player, int poolId)
{
    std::vector<GachaItem> results;
    results.reserve(10);

    for (int i = 0; i < 10; ++i)
    {
        results.push_back(DrawOnce(player, poolId));
    }

    return results;
}
