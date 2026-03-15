#pragma once

#include <vector>
#include <unordered_map>

class Inventory
{
public:
    // 增加 count 参数，支持一次性获得多个物品
    void AddItem(int itemId, int count = 1);

    // 获取特定物品的数量
    int GetItemCount(int itemId) const;

    const std::unordered_map<int, int>& GetItems() const;

    
private:
    // itemId -> count (堆叠逻辑)
    std::unordered_map<int, int> items_;
};