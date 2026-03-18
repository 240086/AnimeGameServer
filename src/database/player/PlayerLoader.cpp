#include "database/player/PlayerLoader.h"
#include "database/mysql/MySQLConnectionPool.h"
#include "database/redis/RedisClient.h" // 假设已包含
#include "common/logger/Logger.h"
#include <vector>
#include <nlohmann/json.hpp> // 建议引入 json 库
#include "database/redis/RedisPool.h"

namespace
{
    constexpr size_t MAX_GACHA_HISTORY_LOAD = 500;
    constexpr int REDIS_CACHE_EXPIRE = 3600; // 缓存 1 小时
}

// 辅助函数：统一获取 Redis 客户端单例（建议后续升级为 Pool）
static RedisClient &GetRedis()
{
    static RedisClient redis;
    static bool inited = redis.Connect("127.0.0.1", 6379);
    return redis;
}

bool PlayerLoader::Load(uint64_t playerId, Player &player)
{
    // 1. 加载货币（Redis 优先）
    if (!LoadCurrency(playerId, player))
        return false;

    // 2. 加载背包（Redis 优先）
    if (!LoadInventory(playerId, player))
        return false;

    // 3. 加载抽卡历史（直连 MySQL，冷数据不进 Redis）
    if (!LoadGachaHistory(playerId, player))
        return false;

    // 清理脏标记，防止登录即存库
    (void)player.FetchDirtyFlags();

    LOG_INFO("Player {} context built successfully", playerId);
    return true;
}

bool PlayerLoader::LoadCurrency(uint64_t playerId, Player &player)
{
    std::string key = "p:" + std::to_string(playerId) + ":cur";
    std::string lockKey = key + ":lock";

    // --- 1. 快路径：常规缓存读取 ---
    {
        RedisGuard redis;
        auto val = redis->Get(key);
        if (val.has_value())
        {
            player.GetCurrency().Set(std::stoull(*val));
            return true;
        }
    }

    // --- 2. 缓存缺失，尝试加锁抢占查询权 ---
    bool gotLock = false;
    {
        RedisGuard redis;
        // 锁定 3 秒，足以覆盖一次 MySQL 查询
        gotLock = redis->SetNX(lockKey, "1", 3);
    }

    if (gotLock)
    {
        // --- 3. Double Check (再次确认缓存) ---
        // 可能在你抢锁的过程中，前一个抢到锁的人已经把数据填好了
        {
            RedisGuard redis;
            auto val = redis->Get(key);
            if (val.has_value())
            {
                player.GetCurrency().Set(std::stoull(*val));
                // 这里可以选删掉 lockKey，也可以等它 3 秒自动过期
                return true;
            }
        }

        // --- 4. 查库 ---
        auto conn = MySQLConnectionPool::Instance().Acquire();
        if (!conn)
            return false;

        std::string sql = "SELECT amount FROM player_currency WHERE player_id=" +
                          std::to_string(playerId) + " AND currency_type=1";

        auto result = conn->Query(sql);
        uint64_t amount = 0;
        if (result)
        {
            auto row = result->FetchRow();
            if (row && row[0])
                amount = std::stoull(row[0]);
        }

        player.GetCurrency().Set(amount);

        // --- 5. 回填缓存 ---
        {
            RedisGuard redis;
            redis->Set(key, std::to_string(amount), REDIS_CACHE_EXPIRE);
        }

        return true;
    }
    else
    {
        // --- 6. 没抢到锁：进入等待自旋 ---
        // 这里的逻辑是：既然有人在查，我就等一会再看缓存
        for (int i = 0; i < 5; ++i) // 增加到 5 次自旋
        {
            // 每次等待 10ms，给数据库查询留出时间
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            RedisGuard redis;
            auto val = redis->Get(key);
            if (val.has_value())
            {
                player.GetCurrency().Set(std::stoull(*val));
                return true;
            }
        }

        // --- 7. 最终兜底 ---
        // 如果等了 50ms 还没数据（可能抢到锁的线程挂了），则直接查库，不再等待
        LOG_WARN("Cache stampede fallback to DB for player {}", playerId);
        auto conn = MySQLConnectionPool::Instance().Acquire();
        if (!conn)
            return false;

        auto result = conn->Query("SELECT amount FROM player_currency WHERE player_id=" + std::to_string(playerId));
        if (result)
        {
            auto row = result->FetchRow();
            uint64_t amount = (row && row[0]) ? std::stoull(row[0]) : 0;
            player.GetCurrency().Set(amount);
        }
        return true;
    }
}

bool PlayerLoader::LoadInventory(uint64_t playerId, Player &player)
{
    std::string key = "p:" + std::to_string(playerId) + ":inv";
    RedisGuard redis;

    // 1. Redis JSON 解析
    auto val = redis->Get(key);
    if (val.has_value())
    {
        try
        {
            auto j = nlohmann::json::parse(*val);
            for (auto &[id_str, count] : j.items())
            {
                player.GetInventory().AddItem(std::stoi(id_str), count.get<int>());
            }
            return true;
        }
        catch (...)
        {
            LOG_WARN("Redis inventory parse error, playerId={}", playerId);
        }
    }

    // 2. MySQL Fallback
    auto conn = MySQLConnectionPool::Instance().Acquire();
    if (!conn)
        return false;

    std::string sql = "SELECT item_id, count FROM player_inventory WHERE player_id=" + std::to_string(playerId);
    auto result = conn->Query(sql);
    if (!result)
        return false;

    nlohmann::json j_backfill = nlohmann::json::object();
    MYSQL_ROW row;
    while ((row = result->FetchRow()))
    {
        int id = std::stoi(row[0]);
        int count = std::stoi(row[1]);
        player.GetInventory().AddItem(id, count);
        j_backfill[std::to_string(id)] = count;
    }

    // 3. 回填
    if (!j_backfill.empty())
    {
        redis->Set(key, j_backfill.dump(), REDIS_CACHE_EXPIRE);
    }
    return true;
}

bool PlayerLoader::LoadGachaHistory(uint64_t playerId, Player &player)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();
    if (!conn)
        return false;

    // DESC LIMIT 获取最新切片
    std::string sql = "SELECT rarity FROM gacha_history WHERE player_id=" +
                      std::to_string(playerId) +
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

    // 逆序回放：[新 -> 旧] 变为内存中的 [旧 -> 新]
    for (auto it = rarities.rbegin(); it != rarities.rend(); ++it)
    {
        player.GetGachaHistory().Record(*it);
    }

    // 设置持久化游标，避免重复加载的数据被误判为“待存入数据”
    if (!rarities.empty())
    {
        // 注意：这里需要你的 GachaHistory 逻辑支持根据最后一条数据设置序列号
        // 假设 Record 内部会自动维护序列号
    }

    return true;
}