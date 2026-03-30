#include "game/gacha/GachaSystem.h"
#include "common/logger/Logger.h"
#include "game/gacha/PitySystem.h"
#include "game/player/Player.h"
#include "game/gacha/GachaPoolManager.h"
#include <stdexcept>

GachaSystem &GachaSystem::Instance()
{
    static GachaSystem instance;
    return instance;
}

/**
 * 单抽入口：负责前置校验和池子定位
 */
GachaItem GachaSystem::DrawOnce(Player &player, int poolId)
{
    int targetPoolId = (poolId <= 0) ? default_pool_id_ : poolId;

    // 使用 shared_ptr 快照持有池子生命周期，避免热重载时引用悬空
    auto pool = GachaPoolManager::Instance().GetPool(targetPoolId);

    return DrawOnceFromPool(player, *pool);
}

/**
 * 十连抽入口：性能优化版，避免重复查找池子
 */
std::vector<GachaItem> GachaSystem::DrawTen(Player &player, int poolId)
{
    std::vector<GachaItem> results;
    results.reserve(10);

    int targetPoolId = (poolId <= 0) ? default_pool_id_ : poolId;
    auto pool = GachaPoolManager::Instance().GetPool(targetPoolId);

    for (int i = 0; i < 10; ++i)
    {
        // 核心优化：直接传递 pool 引用，避免循环内 10 次 Map 查找
        results.push_back(DrawOnceFromPool(player, *pool));
    }

    return results;
}

/**
 * 内部核心逻辑：执行具体的随机过程并记录保底
 * 隔离了外部池子查找逻辑，专注于业务实现
 */
GachaItem GachaSystem::DrawOnceFromPool(Player &player, const GachaPool &pool)
{
    auto &history = player.GetGachaHistory();

    // 1. 根据保底状态决定本次掉落的稀有度
    // TODO: 如果要实现多池保底独立，此处需改为 RollRarity(history, pool.poolId)
    int rarity = PitySystem::RollRarity(history);

    // 2. 防御性回退：若配置缺少某个稀有度，按高到低回退，避免直接抛错
    int actualRarity = rarity;
    while (actualRarity >= 3 && !pool.HasRarity(actualRarity))
    {
        --actualRarity;
    }

    if (actualRarity < 3)
    {
        throw std::runtime_error("gacha pool has no drawable rarity (3/4/5 all empty)");
    }

    if (actualRarity != rarity)
    {
        LOG_WARN("Rolled rarity {} but missing in pool, fallback to rarity {}", rarity, actualRarity);
    }

    // 3. 根据最终稀有度从池子抽取物品，成功后再更新保底，避免失败污染 pity 计数
    auto item = pool.DrawByRarity(actualRarity);

    // 4. 记录本次抽卡结果到保底系统（以实际发放稀有度为准）
    history.Record(item.rarity);

    return item;
}