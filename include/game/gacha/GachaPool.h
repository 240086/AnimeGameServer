#pragma once
// F:\VSCode_project\Cpp_Proj\AnimeGameServer\include\game\gacha\GachaPool.h
#include <vector>
#include "game/gacha/GachaItem.h"

class GachaPool
{
public:

    void AddItem(const GachaItem& item);

    GachaItem Draw();

private:

    std::vector<GachaItem> items_;
    int total_weight_ = 0;
};