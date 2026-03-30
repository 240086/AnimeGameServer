#pragma once

#include <cstdint>
#include <memory>

class Session;
class Player;
class Connection;

enum class ProtocolType
{
    CLIENT,  // 外网（客户端）
    INTERNAL // 内网（网关/服务间）
};

#pragma pack(push, 1)
struct MessageContext
{
    uint32_t sid;
    uint32_t seqId;
    uint16_t msgId;
    ProtocolType protoType = ProtocolType::INTERNAL;

    std::shared_ptr<Session> session;
    std::shared_ptr<Player> player;
    std::shared_ptr<Connection> conn;

};
#pragma pack(pop)