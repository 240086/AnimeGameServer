#pragma once

#include <memory>
#include "game/player/Player.h"

class PlayerRepository
{
public:
    static std::shared_ptr<Player> LoadPlayer(Player::PlayerId playerId);

    static bool SaveInventory(Player &player);

    static bool SaveCurrency(const Player &player);

    static bool InsertGachaRecord(Player::PlayerId playerId, int itemId, int rarity);
};