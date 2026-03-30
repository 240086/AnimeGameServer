#include "game/gacha/GachaPool.h"
#include "common/random/RandomEngine.h"
#include <stdexcept>

void GachaPool::AddItem(const GachaItem &item)
{
    rarity_items_[item.rarity].push_back(item);

    rarity_weight_[item.rarity] += item.weight;
}

GachaItem GachaPool::DrawByRarity(int rarity) const
{
    auto it = rarity_items_.find(rarity);

    if (it == rarity_items_.end() || it->second.empty())
    {
        throw std::runtime_error("rarity pool empty");
    }

    auto &items = it->second;

    auto wIt = rarity_weight_.find(rarity);
    int totalWeight = (wIt == rarity_weight_.end()) ? 0 : wIt->second;
    if (totalWeight <= 0)
    {
        throw std::runtime_error("rarity pool has invalid total weight");
    }

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

bool GachaPool::HasRarity(int rarity) const
{
    auto it = rarity_items_.find(rarity);
    return it != rarity_items_.end() && !it->second.empty();
}