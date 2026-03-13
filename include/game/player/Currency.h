#pragma once

#include <atomic>

class Currency
{
public:

    bool Spend(int amount);

    void Add(int amount);

    int Get() const;

private:

    std::atomic<int> value_{0};
};