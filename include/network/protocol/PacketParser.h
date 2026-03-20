#pragma once

#include "network/buffer/RecvBuffer.h"
#include <functional>

struct MessageContext
{
    uint32_t sessionId;
    uint16_t msgId;
    // 未来可以扩展，比如 timestamp, sourceNodeId 等
};

class PacketParser
{
public:
    using PacketCallback = std::function<void(const MessageContext &, const char *, size_t)>;

    void Parse(RecvBuffer &buffer, PacketCallback callback);

private:
    static constexpr size_t HEADER_SIZE = 10; // Len(4) + Sid(4) + Id(2)
};