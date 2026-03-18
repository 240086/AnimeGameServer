#pragma once

#include "game/actor/Actor.h"
#include "game/player/Player.h"
#include <memory>

class PlayerActor : public Actor
{
public:
    explicit PlayerActor(std::shared_ptr<Player> player)
        : player_(player)
    {
    }

    std::shared_ptr<Player> GetPlayer()
    {
        return player_;
    }

    // 🔥 核心：稳定路由
    uint64_t GetRoutingKey() const override
    {
        return player_ ? player_->GetId() : 0;
    }

private:
    std::shared_ptr<Player> player_;
};