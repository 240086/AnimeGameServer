#include "game/gacha/GachaSystem.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "common/logger/Logger.h"
#include "game/gacha/PitySystem.h"
#include "game/player/Player.h"
#include "game/gacha/GachaPoolManager.h"
#include "game/gacha/GachaSystem.h"

GachaSystem::GachaSystem()
{
    // pool_.AddItem({1, "5-star character", 5, 6});
    // pool_.AddItem({2, "4-star character", 4, 50});
    // pool_.AddItem({3, "3-star weapon", 3, 944});
}

GachaSystem &GachaSystem::Instance()
{
    static GachaSystem instance;
    return instance;
}

GachaItem GachaSystem::DrawOnce(Player &player)
{
    auto &history = player.GetGachaHistory();

    int rarity = PitySystem::RollRarity(history);

    // 2. 【缺失的关键步骤】记录本次结果，触发保底计数器更新和窗口滑动
    history.Record(rarity);

    auto &pool =
        GachaPoolManager::Instance().GetPool(default_pool_id_);

    auto item = pool.DrawByRarity(rarity);

    return item;
}

std::vector<GachaItem> GachaSystem::DrawTen(Player &player)
{
    std::vector<GachaItem> results;

    results.reserve(10);

    for (int i = 0; i < 10; i++)
    {
        auto item = DrawOnce(player);
        results.push_back(item);
    }

    return results;
}
