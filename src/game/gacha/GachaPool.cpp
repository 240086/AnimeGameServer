// F:\VSCode_project\Cpp_Proj\AnimeGameServer\src\game\gacha\GachaPool.cpp
#include "game/gacha/GachaPool.h"
#include "common/random/RandomEngine.h"

void GachaPool::AddItem(const GachaItem &item)
{
    items_.push_back(item);
    total_weight_ += item.weight;
}

GachaItem GachaPool::Draw()
{
    if (items_.empty()) {
        throw("GachaPool empty!!!");
        return {}; // 或者抛出异常
    }
    int r = RandomEngine::Instance().RandInt(1, total_weight_);

    int cumulative = 0;

    for (auto &item : items_)
    {
        cumulative += item.weight;

        if (r <= cumulative)
        {
            return item;
        }
    }

    return items_.back();
}