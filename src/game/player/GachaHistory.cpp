#include "game/player/GachaHistory.h"
#include "game/player/Player.h"
#include "common/logger/Logger.h"

void GachaHistory::Record(int rarity)
{
#ifndef NDEBUG
    if (recording_)
    {
        LOG_ERROR("Duplicate gacha history recording detected");
        return;
    }
    recording_ = true;
#endif

    GachaRecord rec;
    rec.seq = nextSeq_++;
    rec.rarity = rarity;

    history_.push_back(rec);

    if (history_.size() > MAX_HISTORY)
    {
        history_.pop_front();
    }

    if (rarity == 5)
        pityCount_ = 0;
    else
        pityCount_++;

    if (owner_)
    {
        owner_->MarkDirty(PlayerDirtyFlag::GACHA_HISTORY);
    }
#ifndef NDEBUG
    recording_ = false;
#endif
}

std::vector<GachaRecord> GachaHistory::GetUnpersisted() const
{
    std::vector<GachaRecord> result;

    for (const auto &rec : history_)
    {
        if (rec.seq > persistedSeq_)
        {
            result.push_back(rec);
        }
    }

    return result;
}

void GachaHistory::MarkPersisted(uint64_t seq)
{
    if (seq > persistedSeq_)
    {
        persistedSeq_ = seq;
    }
}