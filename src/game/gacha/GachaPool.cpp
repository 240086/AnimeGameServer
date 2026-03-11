#include "game/gacha/GachaPool.h"
#include "common/random/RandomEngine.h"

void GachaPool::AddItem(const GachaItem& item)
{
    items_.push_back(item);
}

GachaItem GachaPool::Draw()
{
    double r = RandomEngine::Instance().RandDouble(0.0, 1.0);

    double cumulative = 0.0;

    for (auto& item : items_)
    {
        cumulative += item.probability;

        if (r <= cumulative)
        {
            return item;
        }
    }

    return items_.back();
}