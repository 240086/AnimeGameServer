#pragma once

enum MessageId
{
    // client -> server

    MSG_LOGIN = 1,
    MSG_GACHA = 2,
    MSG_HEARTBEAT = 3,
    MSG_GACHA_TEN = 4,
    MSG_GACHA_HISTORY = 5,

    // server -> client

    MSG_LOGIN_RESPONSE = 1000,

    MSG_GACHA_DRAW = 1001,
    MSG_GACHA_DRAW_TEN = 1002,

    MSG_GACHA_HISTORY_RESPONSE = 1003
};