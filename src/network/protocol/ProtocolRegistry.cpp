// src/network/protocol/ProtocolRegistry.cpp
#include "network/protocol/ProtocolRegistry.h"
#include "network/protocol/MessageMacro.h"
#include "network/protocol/MessageId.h"
#include "common/logger/Logger.h"

void ProtocolRegistry::RegisterAll()
{
    // --- 登录模块 ---
    REGISTER_PROTO_MESSAGE(MSG_C2S_LOGIN, anime::LoginRequest);

    // --- 抽卡模块 ---
    REGISTER_PROTO_MESSAGE(MSG_C2S_GACHA_DRAW, anime::GachaDrawRequest);
    REGISTER_PROTO_MESSAGE(MSG_C2S_GACHA_DRAW_TEN, anime::GachaDrawRequest);

    // --- 心跳模块 ---
    REGISTER_PROTO_MESSAGE(MSG_C2S_HEARTBEAT, anime::HeartbeatRequest);

    LOG_INFO("ProtocolRegistry: all protocols registered");
}