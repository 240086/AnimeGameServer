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

/**
 * 单抽入口：负责前置校验和池子定位
 */
GachaItem GachaSystem::DrawOnce(Player &player, int poolId)
{
    int targetPoolId = (poolId <= 0) ? default_pool_id_ : poolId;

    // 从 Manager 获取池子引用，如果池子不存在，GetPool 内部会有 Fallback 兜底
    auto &pool = GachaPoolManager::Instance().GetPool(targetPoolId);

    return DrawOnceFromPool(player, pool);
}

/**
 * 十连抽入口：性能优化版，避免重复查找池子
 */
std::vector<GachaItem> GachaSystem::DrawTen(Player &player, int poolId)
{
    std::vector<GachaItem> results;
    results.reserve(10);

    int targetPoolId = (poolId <= 0) ? default_pool_id_ : poolId;
    auto &pool = GachaPoolManager::Instance().GetPool(targetPoolId);

    for (int i = 0; i < 10; ++i)
    {
        // 核心优化：直接传递 pool 引用，避免循环内 10 次 Map 查找
        results.push_back(DrawOnceFromPool(player, pool));
    }

    return results;
}

/**
 * 内部核心逻辑：执行具体的随机过程并记录保底
 * 隔离了外部池子查找逻辑，专注于业务实现
 */
GachaItem GachaSystem::DrawOnceFromPool(Player &player, GachaPool &pool)
{
    auto &history = player.GetGachaHistory();

    // 1. 根据保底状态决定本次掉落的稀有度
    // TODO: 如果要实现多池保底独立，此处需改为 RollRarity(history, pool.poolId)
    int rarity = PitySystem::RollRarity(history);

    // 2. 记录本次抽卡结果到保底系统中
    // 注意：只在这里记录一次，避免服务层重复 Record 导致计数失真
    history.Record(rarity);

    // 3. 根据稀有度从具体池子中随机出一个物品
    // 这里的 DrawByRarity 内部应实现我们之前讨论的 UP/常驻 权重随机
    return pool.DrawByRarity(rarity);
}