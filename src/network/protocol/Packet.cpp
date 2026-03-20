#include "network/protocol/Packet.h"
#include <cstring>
#include <boost/asio/detail/socket_ops.hpp>
#include "network/protocol/Packet.h"

namespace socket_ops = boost::asio::detail::socket_ops;

static constexpr size_t HEADER_SIZE = 10;

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

void Packet::Append(const char *data, size_t len)
{
    buffer_.insert(buffer_.end(), data, data + len);
    header_.length = buffer_.size();
}

void Packet::Append(const std::string &s)
{
    Append(s.data(), s.size());
}
const std::vector<char> &Packet::GetBuffer() const
{
    return buffer_;
}

std::vector<char> Packet::Serialize(uint32_t sessionId) const
{
    std::vector<char> out;

    out.resize(HEADER_SIZE + buffer_.size());

    // 使用 Boost 提供的跨平台转换函数
    // host_to_network_long  等同于 htonl
    // host_to_network_short 等同于 htons
    uint32_t totalLen = socket_ops::host_to_network_long(6 + buffer_.size());
    uint32_t netSid = socket_ops::host_to_network_long(sessionId);
    uint16_t netId = socket_ops::host_to_network_short(header_.messageId);

    std::memcpy(out.data(), &totalLen, 4);
    std::memcpy(out.data() + 4, &netSid, 4);
    std::memcpy(out.data() + 8, &netId, 2);

    if (!buffer_.empty())
    {
        std::memcpy(out.data() + HEADER_SIZE, buffer_.data(), buffer_.size());
    }

    return out;
}