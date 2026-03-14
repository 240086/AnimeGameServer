#pragma once
// F:\VSCode_project\Cpp_Proj\AnimeGameServer\include\game\actor\PlayerActor.h
#include "game/actor/Actor.h"
#include "game/player/Player.h"
#include <memory>

class PlayerActor : public Actor
{
public:

    explicit PlayerActor(std::shared_ptr<Player> player)
        : player_(player)
    {}

    std::shared_ptr<Player> GetPlayer()
    {
        return player_;
    }

private:

    std::shared_ptr<Player> player_;
};