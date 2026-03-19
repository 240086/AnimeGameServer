// F:\VSCode_project\Cpp_Proj\AnimeGameServer\include\services\GachaService.h
#pragma once

#include "services/BaseService.h"
#include "network/Connection.h"
#include "network/protocol/IMessage.h" // 引入 IMessage
#include "game/player/Player.h" // 引入 Player
#include <memory>

struct GachaResponse
{
    uint32_t itemId;
    uint8_t rarity;
};

class GachaService : public BaseService
{
public:
    static GachaService& Instance();

    void Init() override;

    // 🔥 升级签名：直接接收 Player 和 解码后的 Message
    void HandleGacha(Connection* conn, Player* player, std::shared_ptr<IMessage> msg);
    void HandleGachaTen(Connection* conn, Player* player, std::shared_ptr<IMessage> msg);
};