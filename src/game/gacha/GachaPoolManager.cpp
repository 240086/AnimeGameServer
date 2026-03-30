#include "game/gacha/GachaPoolManager.h"
#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <string>
#include "common/logger/Logger.h"

GachaPoolManager &GachaPoolManager::Instance()
{
    static GachaPoolManager instance;
    return instance;
}

bool GachaPoolManager::LoadConfig(const std::string &path)
{
    YAML::Node config;
    try
    {
        config = YAML::LoadFile(path);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Failed to load gacha config file: {}, err={}", path, e.what());
        return false;
    }

    auto poolsNode = config["pools"];
    if (!poolsNode || !poolsNode.IsSequence())
    {
        LOG_ERROR("Gacha config invalid: missing 'pools' sequence, path={}", path);
        return false;
    }

    // 清理旧数据，支持热重载
    pools_.clear();

    for (auto poolNode : poolsNode)
    {
        if (!poolNode["pool_id"])
            continue;

        int poolId = poolNode["pool_id"].as<int>();
        std::string poolName = poolNode["name"] ? poolNode["name"].as<std::string>() : ("pool_" + std::to_string(poolId));

        auto pool = std::make_unique<GachaPool>();
        int itemCount = 0;

        // 策略 1：兼容旧配置 items: [{id, name, rarity, weight}]
        auto items = poolNode["items"];
        if (items && items.IsSequence())
        {
            for (auto itemNode : items)
            {
                GachaItem item;
                item.id = itemNode["id"].as<int>();
                item.name = itemNode["name"] ? itemNode["name"].as<std::string>() : ("item_" + std::to_string(item.id));
                item.rarity = itemNode["rarity"].as<int>();
                item.weight = itemNode["weight"].as<int>();

                pool->AddItem(item);
                ++itemCount;
            }
        }
        else
        {
            // 策略 2：兼容生产级分层配置 (up_items / standard_items / all_items)
            // 内部 Lambda 处理不同分组的加载
            auto addRarityItems = [&](const YAML::Node &groupNode, const char *field, int rarity, int defaultWeight)
            {
                if (!groupNode || !groupNode[field] || !groupNode[field].IsSequence())
                    return;

                for (const auto &idNode : groupNode[field])
                {
                    GachaItem item;
                    item.id = idNode.as<int>();
                    item.name = "item_" + std::to_string(item.id);
                    item.rarity = rarity;
                    item.weight = defaultWeight;

                    pool->AddItem(item);
                    ++itemCount;
                }
            };

            // 处理常驻池格式 (all_items)
            if (poolNode["all_items"])
            {
                auto all = poolNode["all_items"];
                addRarityItems(all, "five_star", 5, 100);
                addRarityItems(all, "four_star", 4, 100);
                addRarityItems(all, "three_star", 3, 100);
            }

            // 处理活动/UP池格式 (up_items + standard_items)
            if (poolNode["up_items"] || poolNode["standard_items"])
            {
                auto up = poolNode["up_items"];
                auto std = poolNode["standard_items"];

                // 5星分配：UP 80% 权重, 常驻 20% 权重 (示例比例)
                addRarityItems(up, "five_star", 5, 800);
                addRarityItems(std, "five_star", 5, 200);

                // 4星分配：UP 70% 权重, 其他 30% 权重
                addRarityItems(up, "four_star", 4, 700);
                addRarityItems(std, "four_star", 4, 300);

                // 3星统一放入 standard
                addRarityItems(std, "three_star", 3, 100);
            }
        }

        if (itemCount == 0)
        {
            LOG_WARN("Skip empty gacha pool: {}({})", poolId, poolName);
            continue;
        }

        pools_[poolId] = std::move(pool);
        LOG_INFO("Successfully loaded gacha pool {}({}) with {} items", poolId, poolName, itemCount);
    }

    if (pools_.empty())
    {
        LOG_ERROR("Zero valid gacha pools loaded from {}", path);
        return false;
    }

    return true;
}

GachaPool &GachaPoolManager::GetPool(int poolId)
{
    auto it = pools_.find(poolId);
    if (it != pools_.end())
    {
        return *(it->second);
    }

    // 防御性编程：优先回退到 pool 1 (通常是常驻池)
    auto fallback = pools_.find(1);
    if (fallback != pools_.end())
    {
        LOG_WARN("GachaPool {} not found, fallback to pool 1", poolId);
        return *(fallback->second);
    }

    // 最后兜底：返回已加载的第一个池子，防止 .at() 或迭代器解引用崩溃
    if (!pools_.empty())
    {
        LOG_WARN("GachaPool {} not found, fallback to first available pool {}", poolId, pools_.begin()->first);
        return *(pools_.begin()->second);
    }

    throw std::runtime_error("No gacha pools loaded in GachaPoolManager");
}

bool GachaPoolManager::HasPool(int poolId) const
{
    return pools_.find(poolId) != pools_.end();
}