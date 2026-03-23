#pragma once

#include "network/protocol/MessageContext.h"
#include "network/protocol/InternalPacket.h"
#include "common.pb.h"
#include "common/logger/Logger.h"
#include "common/ErrorCode.h"
#define DEBUG

class ErrorSender
{
public:
    static void Send(const MessageContext &ctx,
                     ErrorCode code,
                     const std::string &msg = "")
    {
        if (!ctx.conn)
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

        InternalPacket pkt(ctx.sid, MSG_S2C_ERROR, ctx.seqId);
        pkt.Append(payload);

        auto data = std::make_shared<std::vector<char>>(pkt.Serialize());
        ctx.conn->SendRaw(data);
    }
};