#pragma once

#include <functional>
#include <unordered_map>
#include <cstdint>
#include <mutex>
#include <memory>
#include "network/protocol/IMessage.h"

class Connection;
class Player;
using MessageHandler = std::function<void(Connection *, Player *, std::shared_ptr<IMessage>)>;

class MessageDispatcher
{
public:
    static MessageDispatcher &Instance();

    void RegisterHandler(uint16_t msgId, MessageHandler handler);

    void Dispatch(uint16_t msgId, Connection *conn, const char *data, size_t len);

private:
    std::unordered_map<uint16_t, MessageHandler> handlers_;

    std::mutex mutex_;
};