#pragma once

#include <functional>
#include <unordered_map>
#include <cstdint>

class Connection;

using MessageHandler = std::function<void(Connection*, const char*, size_t)>;

class MessageDispatcher
{
public:

    static MessageDispatcher& Instance();

    void RegisterHandler(uint16_t msgId, MessageHandler handler);

    void Dispatch(uint16_t msgId, Connection* conn, const char* data, size_t len);

private:

    std::unordered_map<uint16_t, MessageHandler> handlers_;
};