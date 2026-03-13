#include "network/protocol/PacketParser.h"
#include <cstring>
#include <boost/asio/detail/socket_ops.hpp>
#include "common/logger/Logger.h"

namespace socket_ops = boost::asio::detail::socket_ops;

// 建议定义在头文件或配置中
const uint32_t MAX_PACKET_SIZE = 65536; // 64KB

void PacketParser::Parse(RecvBuffer &buffer, PacketCallback callback)
{
    while (true)
    {
        // 1. 检查包头是否完整
        if (buffer.Size() < HEADER_SIZE)
        {
            return;
        }

        const char *data = buffer.Data();
        uint32_t length;
        uint16_t msgId;

        std::memcpy(&length, data, 4);
        std::memcpy(&msgId, data + 4, 2);

        length = socket_ops::network_to_host_long(length);
        msgId = socket_ops::network_to_host_short(msgId);

        // --- 安全检查：限制最大包长度 ---
        if (length > MAX_PACKET_SIZE)
        {
            // 发现非法数据包！
            // 此时 buffer 里的数据已经不可信了，因为我们无法确定下一个合法的包头在哪
            // 建议：此处可以触发一个错误回调，让上层断开这个恶意 Session
            LOG_ERROR("Packet too large: {} bytes, max: {}", length, MAX_PACKET_SIZE);
            return;
        }

        // 2. 检查 Body 是否完整
        if (buffer.Size() < length + HEADER_SIZE)
        {
            return;
        }

        const char *payload = data + HEADER_SIZE;

        if (callback)
        {
            callback(msgId, payload, length);
        }

        // 3. 消费已处理数据
        buffer.Consume(length + HEADER_SIZE);
    }
}