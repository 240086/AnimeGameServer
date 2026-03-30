#pragma once

#include <deque>
#include <vector>
#include <cstdint>

class Player;

struct GachaRecord
{
    uint32_t seq; // 全局递增ID（关键）
    int rarity;
};

class GachaHistory
{
public:
    static constexpr size_t MAX_HISTORY = 1000;

    void Record(int rarity);

    void SetOwner(Player *owner) { owner_ = owner; }

    int SinceLastFiveStar() const { return pityCount_; }

    const std::deque<GachaRecord> &GetHistory() const { return history_; }

    size_t GetTotalCount() const { return history_.size(); }

    // 获取未持久化数据（更安全）
    std::vector<GachaRecord> GetUnpersisted() const;

    // 标记持久化到某个 seq
    void MarkPersisted(uint32_t seq);

private:
    std::deque<GachaRecord> history_;

    int pityCount_ = 0;

    uint32_t nextSeq_ = 1;      // 下一个序号
    uint32_t persistedSeq_ = 0; // 已持久化到的最大seq

    Player *owner_ = nullptr;
#ifndef NDEBUG
    bool recording_ = false;
#endif
};