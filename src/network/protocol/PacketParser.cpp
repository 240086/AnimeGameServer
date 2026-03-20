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
        uint32_t sessionId; // 新增：提取 SessionId

        // 提取长度 (前4字节)
        std::memcpy(&length, data, 4);
        length = socket_ops::network_to_host_long(length);

        // --- 安全检查 ---
        if (length > MAX_PACKET_SIZE)
        {
            LOG_ERROR("Packet too large: {} bytes", length);
            // 建议：此处应清空 buffer 并断开连接
            return;
        }

        // 2. 检查整体是否完整 (Length 字段在网关定义的是 Sid+Id+Body 的长度，即 6 + BodyLen)
        if (buffer.Size() < length + 4)
        { // +4 是因为 length 字段本身占 4 字节
            return;
        }

        // 提取 SessionID (偏移 4)
        std::memcpy(&sessionId, data + 4, 4);
        sessionId = socket_ops::network_to_host_long(sessionId);

        // 提取 MsgID (偏移 8)
        std::memcpy(&msgId, data + 8, 2);
        msgId = socket_ops::network_to_host_short(msgId);

        // 3. 回调处理
        const char *payload = data + HEADER_SIZE;
        uint32_t payloadLen = length - 6; // 减去 Sid(4) 和 Id(2)

        MessageContext ctx;
        ctx.sessionId = socket_ops::network_to_host_long(sessionId);
        ctx.msgId = socket_ops::network_to_host_short(msgId);

        if (callback)
        {
            const char *payload = data + HEADER_SIZE;
            size_t payloadLen = length - 6; // 减去 Sid(4) 和 Id(2)
            callback(ctx, payload, payloadLen);
        }

        // 4. 消费数据
        buffer.Consume(length + 4);
    }
}