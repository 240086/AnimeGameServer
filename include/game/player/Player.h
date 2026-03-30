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

    // --- DirtyFlag API ---

    // 标记某个模块为脏数据
    void MarkDirty(PlayerDirtyFlag flag);

    // 原子交换：取出当前所有脏标记并清空内存中的标记
    uint32_t FetchDirtyFlags();

    bool IsLoggingOut() const
    {
        return is_logging_out_.load(std::memory_order_acquire);
    }

    bool TryMarkLoggingOut()
    {
        bool expected = false;
        return is_logging_out_.compare_exchange_strong(
            expected, true, std::memory_order_acq_rel);
    }

    // 检查是否有脏数据
    bool IsDirty() const;

    // --- Saving State API ---

    // ✅ 检查是否正在保存中（用于 AutoSaveTick 过滤）
    bool IsSaving() const
    {
        return saving_.load(std::memory_order_acquire);
    }

    // ✅ 尝试进入“保存中状态”
    // 使用 acq_rel 确保：在此之前的内存修改对后续获得锁的线程可见
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

    // 🔥 设置当前合法 SessionId (由 LoginService 调用)
    void SetSessionId(uint32_t sid)
    {
        session_id_.store(sid, std::memory_order_release);
    }

    // 🔥 获取当前合法 SessionId (由 Dispatcher/Actor 校验)
    uint32_t GetSessionId() const
    {
        return session_id_.load(std::memory_order_acquire);
    }

    void LoadFrom(const Player &other);

private:
    PlayerId id_;

    Inventory inventory_;
    GachaHistory history_;
    Currency currency_;

    // 使用原子变量防止多线程 MarkDirty 与 FetchDirty 冲突
    std::atomic<uint32_t> dirtyFlags_{0};

    // 使用原子变量防止重复提交保存任务
    std::atomic<bool> saving_{false};

    std::atomic<bool> is_logging_out_{false};

    std::atomic<uint32_t> session_id_{0};
};