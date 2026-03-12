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

int Currency::Get() const
{
    return value_.load();
}