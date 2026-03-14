enum MessageId : uint16_t
{
    // --- [1 ~ 999] Client -> Server Requests ---
    MSG_C2S_LOGIN = 1,
    MSG_C2S_GACHA_DRAW = 10,     // 单抽
    MSG_C2S_GACHA_DRAW_TEN = 11, // 十连
    MSG_C2S_GACHA_HISTORY = 12,  // 抽卡历史请求

    MSG_C2S_HEARTBEAT = 100,
    MSG_S2C_HEARTBEAT_RESP = 101,

    // --- [1000 ~ 1999] Server -> Client Responses ---
    MSG_S2C_LOGIN_RESP = 1001,
    MSG_S2C_GACHA_DRAW_RESP = 1010,
    MSG_S2C_GACHA_DRAW_TEN_RESP = 1011,
    MSG_S2C_GACHA_HISTORY_RESP = 1012,

    // --- [2000 ~ 2999] Server -> Client Push (预留：服务端主动推送) ---
    // 例如：玩家在其他端登录导致被踢、系统公告、货币变更异步通知
    MSG_S2C_NOTIFY_KICK = 2001,
    MSG_S2C_NOTIFY_CURRENCY = 2002,

    // --- [9000 ~ 9999] Error Codes ---
    MSG_S2C_ERROR_COMMON = 9000,            // 通用错误
    MSG_S2C_ERROR_AUTH_FAIL = 9001,         // 登录验证失败
    MSG_S2C_ERROR_INSUFFICIENT_FUNDS = 9002 // 余额不足
};