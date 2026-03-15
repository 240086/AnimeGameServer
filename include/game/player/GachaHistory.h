#pragma once

#include <vector>

class GachaHistory
{
public:

    void Record(int rarity);

    int SinceLastFiveStar() const;

private:

    std::vector<int> history_;

};