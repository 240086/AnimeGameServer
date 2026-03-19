#pragma once

#include "network/protocol/MessageDecoder.h"
#include "network/protocol/ProtoMessage.h"
#include "common/logger/Logger.h"

class MessageRegistry
{
public:
    template <typename PB>
    static void RegisterProto(uint16_t msgId)
    {
        MessageDecoder::Instance().Register(msgId,
                                            [msgId](const char *data, size_t len) -> std::shared_ptr<IMessage>
                                            {
                                                auto pb = std::make_shared<PB>();

                                                if (!pb->ParseFromArray(data, (int)len))
                                                {
                                                    LOG_ERROR("Proto parse failed, msgId={}", msgId);
                                                    return nullptr;
                                                }

                                                return std::make_shared<
                                                    ProtoMessage<PB>>(msgId, pb);
                                            });
    }
};