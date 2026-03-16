#pragma once

#include <memory>
#include "game/player/Player.h"

class PlayerLoader
{
public:

    static bool Load(uint64_t playerId, Player& player);

private:

    static bool LoadCurrency(uint64_t playerId, Player& player);

    static bool LoadInventory(uint64_t playerId, Player& player);

    static bool LoadGachaHistory(uint64_t playerId, Player& player);
};