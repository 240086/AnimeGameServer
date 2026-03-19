#include "game/player/Player.h"

Player::Player(PlayerId id)
    : id_(id)
{
    // 假设这些组件需要持有 Player 指针用于触发 MarkDirty
    inventory_.SetOwner(this);
    history_.SetOwner(this);
}

Player::PlayerId Player::GetId() const
{
    return id_;
}

Inventory &Player::GetInventory()
{
    return inventory_;
}

GachaHistory &Player::GetGachaHistory()
{
    return history_;
}

Currency &Player::GetCurrency()
{
    return currency_;
}

void Player::MarkDirty(PlayerDirtyFlag flag)
{
    // 使用 fetch_or 保证原子性，多个线程同时修改不同标记位不会互相覆盖
    dirtyFlags_.fetch_or(static_cast<uint32_t>(flag), std::memory_order_relaxed);
}

uint32_t Player::FetchDirtyFlags()
{
    // 使用 exchange(0) 保证清空操作是原子的
    // 使用 acq_rel 建立内存屏障，确保在清除标记前，逻辑层的所有修改已经写入内存
    return dirtyFlags_.exchange(0, std::memory_order_acq_rel);
}

bool Player::IsDirty() const
{
    // 仅仅读取，不需要高强度的屏障
    return dirtyFlags_.load(std::memory_order_relaxed) != 0;
}
void Player::LoadFrom(const Player &other)
{
    this->id_ = other.id_;

    this->inventory_ = other.inventory_;
    this->history_ = other.history_;
    this->currency_ = other.currency_;

    // 🔥 关键修复：重新绑定 owner
    this->inventory_.SetOwner(this);
    this->history_.SetOwner(this);

    // 原子状态重置
    this->dirtyFlags_.store(0, std::memory_order_relaxed);
    this->saving_.store(false, std::memory_order_relaxed);
    this->is_logging_out_.store(false, std::memory_order_relaxed);
}