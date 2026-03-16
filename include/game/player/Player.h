#pragma once

#include <cstdint>
#include <atomic>

#include "game/player/Inventory.h"
#include "game/player/GachaHistory.h"
#include "game/player/Currency.h"
#include "game/player/PlayerDirtyFlag.h"

class Player
{
public:
    using PlayerId = uint64_t;

    explicit Player(PlayerId id);

    PlayerId GetId() const;

    Inventory &GetInventory();

    GachaHistory &GetGachaHistory();

    Currency &GetCurrency();

    // DirtyFlag API
    void MarkDirty(PlayerDirtyFlag flag);

    uint32_t FetchDirtyFlags();

    bool IsDirty() const;

private:
    PlayerId id_;

    Inventory inventory_;
    GachaHistory history_;
    Currency currency_;

    std::atomic<uint32_t> dirtyFlags_{0};
};