#pragma once

#include <memory>
#include "game/player/Player.h"

class PlayerLoader
{
public:
    static bool Load(uint64_t playerId, Player& player);
};