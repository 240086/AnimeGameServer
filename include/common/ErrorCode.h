#pragma once
#include <cstdint>

enum class ErrorCode : uint32_t
{
    OK = 0,

    // 通用
    UNKNOWN = 1,
    INVALID_REQUEST = 2,
    DECODE_FAILED = 3,

    // 认证
    AUTH_FAILED = 1001,
    SESSION_INVALID = 1002,
    PLAYER_LOAD_FAILED = 1003,

    // 经济系统
    INSUFFICIENT_FUNDS = 2001,

    // 抽卡
    GACHA_POOL_INVALID = 3001,

    // 系统
    INTERNAL_ERROR = 9000
};