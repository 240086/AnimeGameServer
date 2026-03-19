#include "database/redis/PlayerCache.h"
#include "database/redis/RedisPool.h"
#include "common/logger/Logger.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

PlayerCache &PlayerCache::Instance()
{
    static PlayerCache instance;
    return instance;
}

std::string PlayerCache::MakeKey(uint64_t playerId)
{
    return "player:data:" + std::to_string(playerId);
}

std::optional<std::shared_ptr<Player>> PlayerCache::Load(uint64_t playerId)
{
    // ✅ 1. 空缓存短路（防穿透）
    if (IsNullCache(playerId))
    {
        return std::nullopt;
    }

    auto redis = RedisPool::Instance().Acquire();
    if (!redis)
        return std::nullopt;

    std::string dataKey = RedisKeyManager::PlayerData(playerId);
    auto val = redis->Get(dataKey);
    RedisPool::Instance().Release(redis);

    if (!val)
        return std::nullopt;

    try
    {
        json j = json::parse(*val);
        auto player = std::make_shared<Player>(playerId);

        if (j.contains("currency"))
        {
            player->GetCurrency().Set(j["currency"].get<uint64_t>());
        }

        if (j.contains("inventory"))
        {
            for (auto &[k, v] : j["inventory"].items())
            {
                uint32_t itemId = static_cast<uint32_t>(std::stoul(k));
                uint32_t count = v.get<uint32_t>();
                player->GetInventory().AddItem(itemId, count);
            }
        }
        LOG_INFO("PlayerCache: Successfully loaded player {} from Redis", playerId);
        return player;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("PlayerCache parse failed playerId={}, err={}", playerId, e.what());
        return std::nullopt;
    }
}

bool PlayerCache::Save(const std::shared_ptr<Player> &player, int ttlSeconds)
{
    // 1. 基础校验
    if (!player)
        return false;

    std::string dataKey = RedisKeyManager::PlayerData(player->GetId());
    std::string nullKey = RedisKeyManager::PlayerNull(player->GetId());

    try
    {
        nlohmann::json j;
        j["ver"] = 1;
        j["currency"] = player->GetCurrency().Get();

        nlohmann::json inv_json = nlohmann::json::object();
        for (const auto &[itemId, itemCount] : player->GetInventory().GetItems())
        {
            inv_json[std::to_string(itemId)] = itemCount;
        }
        j["inventory"] = inv_json;

        auto redis = RedisPool::Instance().Acquire();
        if (!redis)
            return false;

        // 2. 存储数据
        // 使用带有过期时间的 Set 操作（底层对应 Redis 的 SETEX）
        bool ok = redis->Set(
            dataKey,
            j.dump(),
            ttlSeconds // 使用传入的 TTL
        );

        if (!ok)
        {
            LOG_ERROR("PlayerCache::Save failed playerId={}", player->GetId());
        }

        // 3. 关键：如果之前存在“空缓存标记”，现在有了真实数据，应将其删除
        redis->Del(nullKey);

        RedisPool::Instance().Release(redis);
        return ok;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("PlayerCache::Save Exception: {}", e.what());
        return false;
    }
}

void PlayerCache::SetNullCache(uint64_t playerId)
{
    auto redis = RedisPool::Instance().Acquire();
    if (redis)
    {
        // 标记该玩家在库中也不存在，过期时间设为 1 分钟
        std::string nullKey = RedisKeyManager::PlayerNull(playerId);
        redis->Set(nullKey, "1", 60); // 1分钟过期
        RedisPool::Instance().Release(redis);
    }
}

bool PlayerCache::IsNullCache(uint64_t playerId)
{
    auto redis = RedisPool::Instance().Acquire();
    if (!redis)
        return false;

    std::string nullKey = RedisKeyManager::PlayerNull(playerId);
    auto val = redis->Get(nullKey);
    RedisPool::Instance().Release(redis);
    return val.has_value();
}