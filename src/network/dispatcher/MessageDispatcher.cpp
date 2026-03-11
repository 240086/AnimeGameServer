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
    handlers_[msgId] = handler;
}

void MessageDispatcher::Dispatch(uint16_t msgId, Connection *conn, const char *data, size_t len)
{
    auto it = handlers_.find(msgId);

    if (it == handlers_.end())
    {
        LOG_WARN("unknown message {}", msgId);
        return;
    }

    auto handler = it->second;

    GlobalThreadPool::Instance().GetPool().Enqueue(
        [handler, conn, data, len]()
        {
            handler(conn, data, len);
        });
}