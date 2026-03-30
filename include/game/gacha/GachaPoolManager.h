#pragma once

#include <unordered_map>
#include <memory>

#include "game/gacha/GachaPool.h"

class GachaPoolManager
{
public:

    static GachaPoolManager& Instance();

    bool LoadConfig(const std::string& path);

    GachaPool& GetPool(int poolId);

    bool HasPool(int poolId) const;

private:

    std::unordered_map<int, std::unique_ptr<GachaPool>> pools_;
};