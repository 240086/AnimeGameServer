#pragma once

#include "network/Connection.h"
#include "network/protocol/Packet.h"
#include "common/logger/Logger.h"

class ResponseSender
{
public:
    static bool SendPayload(Connection *conn, uint32_t msgId, const std::string &payload)
    {
        if (!conn)
            return false;

        Packet pkt;
        pkt.SetMessageId(msgId);
        pkt.Append(payload);
        conn->SendPacket(pkt);
        return true;
    }

    template <typename ProtoMsg>
    static bool SendProto(Connection *conn, uint32_t msgId, const ProtoMsg &resp)
    {
        if (!conn)
            return false;

        std::string payload;
        if (!resp.SerializeToString(&payload))
        {
            LOG_ERROR("Serialize response failed, msgId={}", msgId);
            return false;
        }
        return SendPayload(conn, msgId, payload);
    }
};