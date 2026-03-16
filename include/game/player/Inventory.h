#pragma once

#include <vector>
#include <unordered_map>

class Player;
class Inventory
{
public:
    // 构造时或初始化时绑定所属玩家
    void SetOwner(Player *owner) { owner_ = owner; }
    // 增加 count 参数，支持一次性获得多个物品
    void AddItem(int itemId, int count = 1);

    // 获取特定物品的数量
    int GetItemCount(int itemId) const;

    const std::unordered_map<int, int> &GetItems() const;

private:
    // itemId -> count (堆叠逻辑)
    std::unordered_map<int, int> items_;
    Player *owner_ = nullptr; // 核心：指向所属的玩家对象
};