#pragma once

#include <vector>
#include <mutex>

class Inventory
{
public:

    void AddItem(int itemId);

    std::vector<int> GetItems() const;

private:

    std::vector<int> items_;

    mutable std::mutex mutex_;
};