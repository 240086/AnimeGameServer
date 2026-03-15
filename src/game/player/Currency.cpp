#include "game/player/Currency.h"

bool Currency::Spend(int amount)
{
    int current = value_.load();

    while (current >= amount)
    {
        if (value_.compare_exchange_weak(current, current - amount))
        {
            return true;
        }
    }

    return false;
}

void Currency::Add(int amount)
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