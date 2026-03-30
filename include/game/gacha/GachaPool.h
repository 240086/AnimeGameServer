#pragma once

#include <vector>
#include <unordered_map>

#include "game/gacha/GachaItem.h"

class GachaPool
{
public:
    void AddItem(const GachaItem &item);

    GachaItem DrawByRarity(int rarity) const;

    bool HasRarity(int rarity) const;

private:
    std::unordered_map<int, std::vector<GachaItem>> rarity_items_;

    std::unordered_map<int, int> rarity_weight_;
};