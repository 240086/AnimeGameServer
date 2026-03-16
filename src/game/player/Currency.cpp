#include "game/player/Currency.h"

bool Currency::Spend(uint64_t amount)
{
    uint64_t current = value_.load();

    while (current >= amount)
    {
        if (value_.compare_exchange_weak(current, current - amount))
        {
            return true;
        }
    }

    return false;
}

void Currency::Add(uint64_t amount)
{
    value_.fetch_add(amount);
}

uint64_t Currency::Get() const
{
    return value_.load();
}

void Currency::Set(uint64_t value){
    value_ = value;
}