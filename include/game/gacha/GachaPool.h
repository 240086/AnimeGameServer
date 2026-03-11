#pragma once

#include <vector>
#include "game/gacha/GachaItem.h"

class GachaPool
{
public:

    void AddItem(const GachaItem& item);

    GachaItem Draw();

private:

    std::vector<GachaItem> items_;
};