#pragma once

#include <cstdint>

#include "game/player/Inventory.h"
#include "game/player/GachaHistory.h"

class Player
{
public:

    using PlayerId = uint64_t;

    explicit Player(PlayerId id);

    PlayerId GetId() const;

    Inventory& GetInventory();

    GachaHistory& GetGachaHistory();

private:

    PlayerId id_;

    Inventory inventory_;

    GachaHistory history_;
};