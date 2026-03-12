#include "game/player/Player.h"

Player::Player(PlayerId id)
    : id_(id)
{
}

Player::PlayerId Player::GetId() const
{
    return id_;
}

Inventory& Player::GetInventory()
{
    return inventory_;
}

GachaHistory& Player::GetGachaHistory()
{
    return history_;
}