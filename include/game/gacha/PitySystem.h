#pragma once

#include <random>

class GachaHistory;

class PitySystem
{
public:

    static int RollRarity(GachaHistory& history);

private:

    static double FiveStarProbability(int pityCount);

    static std::mt19937& RNG();
};