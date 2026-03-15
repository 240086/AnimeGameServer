#pragma once

#include <memory>
#include "game/player/Player.h"

class PlayerRepository
{
public:
    static PlayerRepository& Instance();

    std::shared_ptr<Player> LoadPlayer(uint64_t playerId);

    bool SavePlayer(const Player& player);
};