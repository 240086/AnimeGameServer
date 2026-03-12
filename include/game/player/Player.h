#pragma once

#include <cstdint>
#include <mutex>

#include "game/player/Inventory.h"
#include "game/player/GachaHistory.h"
#include "game/player/Currency.h"
#include "game/player/PlayerCommandQueue.h"

class Player
{
public:
    using PlayerId = uint64_t;

    explicit Player(PlayerId id);

    PlayerId GetId() const;

    Inventory &GetInventory();

    GachaHistory &GetGachaHistory();

    Currency &GetCurrency();

    std::mutex &GetMutex()
    {
        return mutex_;
    }

    PlayerCommandQueue &GetCommandQueue()
    {
        return commandQueue_;
    }

private:
    PlayerId id_;

    Inventory inventory_;

    GachaHistory history_;

    Currency currency_;

    std::mutex mutex_;

    PlayerCommandQueue commandQueue_;
};