#pragma once

#include <functional>
#include <unordered_map>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include "network/protocol/IMessage.h"
#include "network/protocol/PacketParser.h"
#include "network/protocol/MessageContext.h"

class Connection;
class Player;
using MessageHandler = std::function<void(MessageContext, std::shared_ptr<anime::IMessage>)>;

class MessageDispatcher
{
public:
    static MessageDispatcher &Instance();

    void RegisterHandler(uint16_t msgId, MessageHandler handler);

    void Dispatch(MessageContext ctx,
                  const char *data,
                  size_t len);

private:
    std::unordered_map<uint16_t, MessageHandler> handlers_;

    std::shared_mutex mutex_;
};