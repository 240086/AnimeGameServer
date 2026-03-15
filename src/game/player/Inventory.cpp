#include "game/player/Inventory.h"

void Inventory::AddItem(int itemId, int count)
{
    if (count <= 0) return;

    // unordered_map 的 operator[] 会在 key 不存在时自动创建并初始化为 0
    items_[itemId] += count;
}

int Inventory::GetItemCount(int itemId) const
{
    auto it = items_.find(itemId);
    if (it != items_.end()) {
        return it->second;
    }
    return 0;
}

const std::unordered_map<int, int>& Inventory::GetItems() const
{
    return items_;
}