#include "game/player/Inventory.h"
#include "game/player/Player.h"

void Inventory::AddItem(int itemId, int count)
{
    if (count <= 0)
        return;

    // unordered_map 的 operator[] 会在 key 不存在时自动创建并初始化为 0
    items_[itemId] += count;

    // 关键：通知玩家对象，背包数据已脏
    if (owner_)
    {
        // 假设你在 Player 类中定义了相应的位掩码枚举
        owner_->MarkDirty(PlayerDirtyFlag::INVENTORY);
    }
}

int Inventory::GetItemCount(int itemId) const
{
    auto it = items_.find(itemId);
    if (it != items_.end())
    {
        return it->second;
    }
    return 0;
}

const std::unordered_map<int, int> &Inventory::GetItems() const
{
    return items_;
}