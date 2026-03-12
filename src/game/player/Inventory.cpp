#include "game/player/Inventory.h"

void Inventory::AddItem(int itemId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    items_.push_back(itemId);
}

std::vector<int> Inventory::GetItems() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    return items_;
}