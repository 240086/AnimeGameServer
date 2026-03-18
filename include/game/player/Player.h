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

    // ✅ 尝试进入“保存中状态”
    bool TryMarkSaving()
    {
        bool expected = false;
        return saving_.compare_exchange_strong(
            expected,
            true,
            std::memory_order_acq_rel);
    }

    // ✅ 保存完成后调用
    void ClearSaving()
    {
        saving_.store(false, std::memory_order_release);
    }

private:
    PlayerId id_;

    Inventory inventory_;
    GachaHistory history_;
    Currency currency_;

    std::atomic<uint32_t> dirtyFlags_{0};

    std::atomic<bool> saving_{false};
};