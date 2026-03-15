#pragma once

#include <atomic>

class Currency
{
public:
    bool Spend(int amount);

    void Add(int amount);

    void Set(uint64_t value);

    uint64_t Get() const;

private:
    std::atomic<int> value_{0};
};