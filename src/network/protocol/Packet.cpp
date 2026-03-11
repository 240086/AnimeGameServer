#include "network/protocol/Packet.h"

Packet::Packet()
{
    header_.length = 0;
    header_.messageId = 0;
}

void Packet::SetMessageId(uint16_t id)
{
    header_.messageId = id;
}

uint16_t Packet::GetMessageId() const
{
    return header_.messageId;
}

const std::vector<char>& Packet::GetBuffer() const
{
    return buffer_;
}

void Packet::Append(const char* data, size_t len)
{
    buffer_.insert(buffer_.end(), data, data + len);
    header_.length = buffer_.size();
}