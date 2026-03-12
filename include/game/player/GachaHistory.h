#pragma once

#include <vector>
#include <mutex>

class GachaHistory
{
public:

    void Record(int rarity);

    int SinceLastFiveStar() const;

private:

    std::vector<int> history_;

    mutable std::mutex mutex_;
};