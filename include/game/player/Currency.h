#pragma once

#include <atomic>

class Currency
{
public:
    bool Spend(uint64_t amount);

    void Add(uint64_t amount);

    void Set(uint64_t value);

    uint64_t Get() const;

private:
    std::atomic<uint64_t> value_{0};
};