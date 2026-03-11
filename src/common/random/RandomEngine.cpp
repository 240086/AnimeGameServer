#include "common/random/RandomEngine.h"

RandomEngine::RandomEngine()
{
    std::random_device rd;
    rng_ = std::mt19937(rd());
}

RandomEngine& RandomEngine::Instance()
{
    static RandomEngine instance;
    return instance;
}

int RandomEngine::RandInt(int min, int max)
{
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng_);
}

double RandomEngine::RandDouble(double min, double max)
{
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng_);
}