#pragma once

#include <random>

class RandomEngine
{
public:

    static RandomEngine& Instance();

    int RandInt(int min, int max);

    double RandDouble(double min, double max);

private:

    RandomEngine();

private:

    std::mt19937 rng_;
};