#pragma once

#include <cstdint>
#include <vector>

struct PacketHeader
{
    uint32_t length;
    uint16_t messageId;
};

class Packet
{
public:
    Packet();

    void SetMessageId(uint16_t id);

    uint16_t GetMessageId() const;

    const std::vector<char>& GetBuffer() const;

    void Append(const char* data, size_t len);

private:
    PacketHeader header_;
    std::vector<char> buffer_;
};