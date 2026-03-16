#include "game/player/Player.h"

Player::Player(PlayerId id)
    : id_(id)
{
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
    dirtyFlags_.fetch_or(static_cast<uint32_t>(flag), std::memory_order_relaxed);
}

uint32_t Player::FetchDirtyFlags()
{
    return dirtyFlags_.exchange(0, std::memory_order_acq_rel);
}

bool Player::IsDirty() const
{
    return dirtyFlags_.load(std::memory_order_relaxed) != 0;
}