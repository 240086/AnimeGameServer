#pragma once

#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include "game/gacha/GachaPool.h"

class GachaPoolManager
{
public:
    static GachaPoolManager &Instance();

    bool LoadConfig(const std::string &path);

    std::shared_ptr<const GachaPool> GetPool(int poolId) const;

    bool HasPool(int poolId) const;

private:
    std::shared_ptr<const GachaPool> GetFallbackPoolLocked(int poolId) const;

    mutable std::shared_mutex mutex_;
    std::unordered_map<int, std::shared_ptr<const GachaPool>> pools_;
};