#pragma once

#include "network/Connection.h"
#include "network/protocol/Packet.h"
#include "common/ErrorCode.h"
#include "common/logger/Logger.h"

#include "common.pb.h" // ErrorResponse

class ErrorSender
{
public:
    static void Send(Connection *conn, ErrorCode code, const std::string &msg = "")
    {
        if (!conn)
            return;

        anime::ErrorResponse resp;
        resp.set_code(static_cast<uint32_t>(code));

#ifdef DEBUG
        resp.set_message(msg);
#endif

        std::string payload;
        if (!resp.SerializeToString(&payload))
        {
            LOG_ERROR("ErrorResponse serialize failed");
            return;
        }

        Packet pkt;
        pkt.SetMessageId(MSG_S2C_ERROR);
        pkt.Append(payload);

        conn->SendPacket(pkt);
    }
};