#include "network/dispatcher/MessageDispatcher.h"
#include "common/logger/Logger.h"
#include "common/thread/GlobalThreadPool.h"

MessageDispatcher &MessageDispatcher::Instance()
{
    static MessageDispatcher instance;
    return instance;
}

void MessageDispatcher::RegisterHandler(uint16_t msgId, MessageHandler handler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[msgId] = handler;
}

void MessageDispatcher::Dispatch(uint16_t msgId, Connection *conn, const char *data, size_t len)
{
    MessageHandler handler;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = handlers_.find(msgId);

        if (it == handlers_.end())
            return;

        handler = it->second;
    }

    handler(conn, data, len);
}