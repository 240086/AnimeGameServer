#include "game/player/GachaHistory.h"
#include "game/player/Player.h"

static constexpr size_t MAX_HISTORY = 1000;

void GachaHistory::Record(int rarity)
{
    // 1. 更新历史记录
    history_.push_back(rarity);

    // 2. 维持滑动窗口：超过 1000 条则弹出最早的记录
    if (history_.size() > MAX_HISTORY)
    {
        history_.pop_front(); // O(1) 操作
    }

    // 3. 更新保底计数器
    if (rarity == 5)
    {
        pityCount_ = 0; // 出金了，重置
    }
    else
    {
        pityCount_++; // 没出金，累加
    }

    // 4. 【核心优化】：触发脏标记
    // 只要抽卡发生，历史记录和保底计数都发生了变更
    if (owner_)
    {
        owner_->MarkDirty(PlayerDirtyFlag::GACHA_HISTORY);
    }
}
