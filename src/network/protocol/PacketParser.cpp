#include "network/protocol/PacketParser.h"
#include <cstring>

void PacketParser::Parse(RecvBuffer& buffer, PacketCallback callback)
{
    while (true)
    {
        if (buffer.Size() < HEADER_SIZE)
        {
            return;
        }

        const char* data = buffer.Data();

        uint32_t length;
        uint16_t msgId;

        std::memcpy(&length, data, 4);
        std::memcpy(&msgId, data + 4, 2);

        if (buffer.Size() < length + HEADER_SIZE)
        {
            return;
        }

        const char* payload = data + HEADER_SIZE;

        callback(msgId, payload, length);

        buffer.Consume(length + HEADER_SIZE);
    }
}