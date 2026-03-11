#pragma once

#include "network/buffer/RecvBuffer.h"
#include <functional>

class PacketParser
{
public:

    using PacketCallback = std::function<void(uint16_t, const char*, size_t)>;

    void Parse(RecvBuffer& buffer, PacketCallback callback);

private:

    static constexpr size_t HEADER_SIZE = 6;
};