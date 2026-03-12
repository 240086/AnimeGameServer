#pragma once

#include "game/actor/Actor.h"
#include "game/player/Player.h"

class PlayerActor : public Actor
{
public:

    PlayerActor(uint64_t id)
        : player_(id)
    {}

    Player& GetPlayer()
    {
        return player_;
    }

private:

    Player player_;
};