#include "game/gacha/PitySystem.h"
#include "game/player/GachaHistory.h"

#include <random>

std::mt19937& PitySystem::RNG()
{
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

double PitySystem::FiveStarProbability(int pity)
{
    const double base = 0.006;

    if(pity < 75)
        return base;

    double extra = (pity - 74) * 0.06;

    return base + extra;
}

int PitySystem::RollRarity(GachaHistory& history)
{
    int pity = history.SinceLastFiveStar();

    if(pity >= 89)
        return 5;

    std::uniform_real_distribution<> dist(0.0,1.0);

    double roll = dist(RNG());

    if(roll < FiveStarProbability(pity))
        return 5;

    if(roll < 0.051)
        return 4;

    return 3;
}