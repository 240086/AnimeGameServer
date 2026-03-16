#pragma once
#include <cstdint>
#include <type_traits>

enum class PlayerDirtyFlag : uint32_t
{
    NONE = 0,
    BASE = 1 << 0,          // 基础数据 (Level, Exp)
    CURRENCY = 1 << 1,      // 货币
    INVENTORY = 1 << 2,     // 背包
    GACHA_HISTORY = 1 << 3, // 抽卡记录
    ALL = 0xFFFFFFFF        // 全量标记
};

// 辅助函数：类型转换（内部使用）
template <typename T>
constexpr auto to_utype(T e)
{
    return static_cast<std::underlying_type_t<T>>(e);
}

// 1. 按位或 (用于设置标记)
inline PlayerDirtyFlag operator|(PlayerDirtyFlag a, PlayerDirtyFlag b)
{
    return static_cast<PlayerDirtyFlag>(to_utype(a) | to_utype(b));
}

// 2. 复合赋值 (dirty |= BASE)
inline PlayerDirtyFlag &operator|=(PlayerDirtyFlag &a, PlayerDirtyFlag b)
{
    return a = a | b;
}

// 3. 按位与 (用于检查标记)
inline PlayerDirtyFlag operator&(PlayerDirtyFlag a, PlayerDirtyFlag b)
{
    return static_cast<PlayerDirtyFlag>(to_utype(a) & to_utype(b));
}

// 4. 按位取反与按位与 (用于清除标记：dirty &= ~BASE)
inline PlayerDirtyFlag operator~(PlayerDirtyFlag a)
{
    return static_cast<PlayerDirtyFlag>(~to_utype(a));
}

inline PlayerDirtyFlag &operator&=(PlayerDirtyFlag &a, PlayerDirtyFlag b)
{
    return a = static_cast<PlayerDirtyFlag>(to_utype(a) & to_utype(b));
}