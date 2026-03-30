#include "database/player/PlayerLoader.h"
#include "database/mysql/MySQLConnectionPool.h"
#include "database/redis/RedisPool.h"
#include "database/redis/PlayerCache.h"
#include "common/logger/Logger.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

using json = nlohmann::json;

namespace
{
    constexpr size_t MAX_GACHA_HISTORY_LOAD = 500;
    constexpr int REDIS_CACHE_EXPIRE = 3600; // 1 小时
}

bool PlayerLoader::Load(uint64_t playerId, Player &player)
{
    // 1. 检查是否是已知的“不存在玩家” (空缓存保护)
    if (PlayerCache::Instance().IsNullCache(playerId))
    {
        LOG_WARN("Player {} is marked as non-existent in NullCache", playerId);
        return false;
    }

    // ---------- 1. 读聚合缓存 ----------
    auto cached = PlayerCache::Instance().Load(playerId);
    if (cached.has_value())
    {
        player.LoadFrom(*cached.value()); // ✅ 替换 operator=
        LOG_DEBUG("Player {} loaded from aggregated Redis cache", playerId);
        return true;
    }

    LOG_DEBUG("Player {} cache miss, entering load pipeline...", playerId);

    // ---------- 2. 分布式锁（防止击穿） ----------
    std::string lockKey = "lock:player:load:" + std::to_string(playerId);

    auto redis = RedisPool::Instance().Acquire();
    bool locked = false;

    if (redis)
    {
        locked = redis->SetNX(lockKey, "1", 3); // 3秒短锁
        RedisPool::Instance().Release(redis);
    }

    if (!locked)
    {
        // 👉 没抢到锁：等待 + 重试缓存
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        auto retry = PlayerCache::Instance().Load(playerId);
        if (retry.has_value())
        {
            player.LoadFrom(*retry.value());
            LOG_DEBUG("Player {} loaded from cache after wait", playerId);
            return true;
        }

        LOG_WARN("Player {} load contention fallback to DB", playerId);
    }

    // ---------- 3. DB fallback ----------
    if (!LoadCurrency(playerId, player))
        return false;

    if (!LoadInventory(playerId, player))
        return false;

    if (!LoadGachaHistory(playerId, player))
        return false;

    // ---------- 4. 回填缓存（带 TTL） ----------
    auto newPlayer = std::make_shared<Player>(player.GetId());
    newPlayer->LoadFrom(player);

    PlayerCache::Instance().Save(newPlayer, 300); // TTL 传参

    // ---------- 5. 清脏标记 ----------
    (void)player.FetchDirtyFlags();

    LOG_DEBUG("Player {} loaded from DB and cached", playerId);
    return true;
}

bool PlayerLoader::LoadCurrency(uint64_t playerId, Player &player)
{
    std::string key = "p:" + std::to_string(playerId) + ":cur";
    std::string lockKey = key + ":lock";

    // 1. 尝试获取分布式锁 (SETNX)
    auto redis = RedisPool::Instance().Acquire();
    if (!redis)
        return false;

    bool gotLock = redis->SetNX(lockKey, "1", 3); // 锁定3秒

    if (gotLock)
    {
        // 抢到锁的线程：查库并回填
        auto conn = MySQLConnectionPool::Instance().Acquire();
        if (!conn)
        {
            RedisPool::Instance().Release(redis);
            return false;
        }

        auto result = conn->Query("SELECT amount FROM player_currency WHERE player_id=" +
                                  std::to_string(playerId) + " AND currency_type=1");
        uint64_t amount = 0;
        if (result)
        {
            auto row = result->FetchRow();
            if (row && row[0])
                amount = std::stoull(row[0]);
        }

        player.GetCurrency().Set(amount);

        // 更新细粒度缓存并释放连接
        redis->Set(key, std::to_string(amount), REDIS_CACHE_EXPIRE);
        RedisPool::Instance().Release(redis);
        return true;
    }
    else
    {
        // 没抢到锁：自旋等待其他线程回填
        RedisPool::Instance().Release(redis);
        for (int i = 0; i < 5; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            auto r2 = RedisPool::Instance().Acquire();
            auto val = r2->Get(key);
            RedisPool::Instance().Release(r2);

            if (val.has_value())
            {
                player.GetCurrency().Set(std::stoull(*val));
                return true;
            }
        }

        // 兜底：等太久了直接查库
        LOG_WARN("Spin lock timeout for player {}, fallback to DB", playerId);
        auto conn = MySQLConnectionPool::Instance().Acquire();
        auto result = conn->Query("SELECT amount FROM player_currency WHERE player_id=" + std::to_string(playerId));
        if (result)
        {
            auto row = result->FetchRow();
            player.GetCurrency().Set((row && row[0]) ? std::stoull(row[0]) : 0);
        }
        return true;
    }
}

bool PlayerLoader::LoadInventory(uint64_t playerId, Player &player)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();
    if (!conn)
        return false;

    auto result = conn->Query("SELECT item_id, count FROM player_inventory WHERE player_id=" + std::to_string(playerId));
    if (!result)
        return false;

    MYSQL_ROW row;
    while ((row = result->FetchRow()))
    {
        player.GetInventory().AddItem(std::stoi(row[0]), std::stoi(row[1]));
    }
    return true;
}

bool PlayerLoader::LoadGachaHistory(uint64_t playerId, Player &player)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();
    if (!conn)
        return false;

    std::string sql = "SELECT rarity FROM gacha_history WHERE player_id=" + std::to_string(playerId) +
                      " ORDER BY id DESC LIMIT " + std::to_string(MAX_GACHA_HISTORY_LOAD);

    auto result = conn->Query(sql);
    if (!result)
        return false;

    std::vector<int> rarities;
    MYSQL_ROW row;
    while ((row = result->FetchRow()))
    {
        rarities.push_back(std::stoi(row[0]));
    }

    // 逆序回放，保证内存中顺序是 [旧 -> 新]
    for (auto it = rarities.rbegin(); it != rarities.rend(); ++it)
    {
        player.GetGachaHistory().Record(*it);
    }
    return true;
}