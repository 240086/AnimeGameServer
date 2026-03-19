#pragma once

#include <atomic>

class Currency
{
public:
    bool Spend(uint64_t amount);

    void Add(uint64_t amount);

    void Set(uint64_t value);

    uint64_t Get() const;

    Currency() : value_(0) {}

    Currency(const Currency &other)
    {
        value_.store(other.value_.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

    Currency &operator=(const Currency &other)
    {
        if (this != &other)
        {
            // 原子变量必须先 load 出来，再 store 进去
            uint64_t val = other.value_.load(std::memory_order_relaxed);
            this->value_.store(val, std::memory_order_relaxed);
        }
        return *this;
    }

private:
    std::atomic<uint64_t> value_{0};
};