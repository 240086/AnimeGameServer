// src/network/protocol/ProtocolRegistry.cpp
#include "network/protocol/MessageMacro.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/generated/login.pb.h"
#include "network/protocol/generated/gacha.pb.h"

class ProtocolRegistry {
public:
    static void RegisterAll() {
        // --- 登录模块 ---
        REGISTER_PROTO_MESSAGE(MSG_C2S_LOGIN, anime::LoginRequest);
        
        // --- 抽卡模块 ---
        // 注意：即便请求包目前是空的，也要注册，以便 Decoder 生成包装器
        REGISTER_PROTO_MESSAGE(MSG_C2S_GACHA_DRAW, anime::GachaDrawRequest);
        REGISTER_PROTO_MESSAGE(MSG_C2S_GACHA_DRAW_TEN, anime::GachaDrawRequest);

        // --- 社交/其他模块 (预留) ---
        // REGISTER_PROTO_MESSAGE(MSG_C2S_GET_FRIENDS, anime::FriendListRequest);
        
        LOG_INFO("ProtocolRegistry: All protocols registered (Total: 3)");
    }
};