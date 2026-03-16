#pragma once

#include <deque>
#include <vector>

class Player;
class GachaHistory
{
public:
    // 限制最大历史记录，防止内存溢出
    static constexpr size_t MAX_HISTORY = 1000;

    // 记录一次抽卡结果（rarity: 3, 4, 5）
    void Record(int rarity);

    // 绑定所属玩家
    void SetOwner(Player *owner) { owner_ = owner; }

    // 获取距离上一次五星已抽次数（保底计数）
    int SinceLastFiveStar() const { return pityCount_; }

    // 提供给 PlayerSaver 使用的只读接口
    const std::deque<int> &GetHistory() const { return history_; }

    // 调试或存盘用：获取总抽卡数
    size_t GetTotalCount() const { return history_.size(); }

private:
    // 使用 deque 优化头部删除效率
    std::deque<int> history_;

    // 缓存保底计数，避免重复遍历
    int pityCount_ = 0;

    Player *owner_ = nullptr; // 指向所属玩家
};