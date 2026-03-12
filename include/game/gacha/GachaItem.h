#pragma once

#include <string>

struct GachaItem
{
    int id;

    std::string name;

    int rarity;

    int weight;

    // id       物品ID
    // name     名字
    // rarity   稀有度
    // weight   权重

    // 示例
    // 5星  6
    // 4星  50
    // 3星  944
};