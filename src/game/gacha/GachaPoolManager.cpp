#include "game/gacha/GachaPoolManager.h"
#include <yaml-cpp/yaml.h>
#include "common/logger/Logger.h"

GachaPoolManager& GachaPoolManager::Instance()
{
    static GachaPoolManager instance;
    return instance;
}

bool GachaPoolManager::LoadConfig(const std::string& path)
{
    YAML::Node config = YAML::LoadFile(path);

    auto pools = config["pools"];

    for(auto poolNode : pools)
    {
        int poolId = poolNode["pool_id"].as<int>();

        auto pool = std::make_unique<GachaPool>();

        auto items = poolNode["items"];

        for(auto itemNode : items)
        {
            GachaItem item;

            item.id = itemNode["id"].as<int>();
            item.name = itemNode["name"].as<std::string>();
            item.rarity = itemNode["rarity"].as<int>();
            item.weight = itemNode["weight"].as<int>();

            pool->AddItem(item);
        }

        pools_[poolId] = std::move(pool);

        LOG_INFO("load gacha pool {}", poolId);
    }

    return true;
}

GachaPool& GachaPoolManager::GetPool(int poolId)
{
    return *pools_.at(poolId);
}