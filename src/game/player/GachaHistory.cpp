#include "game/player/GachaHistory.h"

void GachaHistory::Record(int rarity)
{
    std::lock_guard<std::mutex> lock(mutex_);

    history_.push_back(rarity);
}

int GachaHistory::SinceLastFiveStar() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    int count = 0;

    for(auto it = history_.rbegin(); it != history_.rend(); ++it)
    {
        if(*it == 5)
            break;

        count++;
    }

    return count;
}