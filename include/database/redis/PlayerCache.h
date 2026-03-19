#pragma once

#include <memory>
#include <optional>
#include <string>
#include "game/player/Player.h"
#include "database/redis/RedisKeyManager.h"

class PlayerCache
{
public:
    static PlayerCache &Instance();

    // 从 Redis 加载
    std::optional<std::shared_ptr<Player>> Load(uint64_t playerId);

    // 保存到 Redis
    bool Save(const std::shared_ptr<Player> &player, int ttlSeconds = 3600);

    void SetNullCache(uint64_t playerId);
    bool IsNullCache(uint64_t playerId);

private:
    PlayerCache() = default;

    std::string MakeKey(uint64_t playerId);
};