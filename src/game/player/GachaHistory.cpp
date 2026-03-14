#include "game/player/GachaHistory.h"

static constexpr size_t MAX_HISTORY = 1000;

void GachaHistory::Record(int rarity)
{
    std::lock_guard<std::mutex> lock(mutex_);

    history_.push_back(rarity);
    if (history_.size() > MAX_HISTORY)
    {
        history_.erase(history_.begin());
    }
}

int GachaHistory::SinceLastFiveStar() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    int count = 0;

    for (auto it = history_.rbegin(); it != history_.rend(); ++it)
    {
        if (*it == 5)
            break;

        count++;
    }

    return count;
}