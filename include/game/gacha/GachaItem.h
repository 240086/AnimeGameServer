#pragma once

#include <string>

struct GachaItem
{
    int id;

    std::string name;

    int rarity;

    double probability;

    // id            物品ID
    // name          名字
    // rarity        稀有度
    // probability   概率

    // 五星角色 0.6 %
    // 四星角色 5 %
    // 三星武器 94.4 %
};