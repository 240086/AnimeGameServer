#include "game/gacha/GachaPool.h"
#include "common/random/RandomEngine.h"
#include <stdexcept>

void GachaPool::AddItem(const GachaItem &item)
{
    rarity_items_[item.rarity].push_back(item);

    rarity_weight_[item.rarity] += item.weight;
}

GachaItem GachaPool::DrawByRarity(int rarity)
{
    auto it = rarity_items_.find(rarity);

    if (it == rarity_items_.end())
    {
        throw std::runtime_error("rarity pool empty");
    }

    auto &items = it->second;

    int totalWeight = rarity_weight_[rarity];

    int r = RandomEngine::Instance().RandInt(1, totalWeight);

    int cumulative = 0;

    for (auto &item : items)
    {
        cumulative += item.weight;

        if (r <= cumulative)
        {
            return item;
        }
    }

    return items.back();
}