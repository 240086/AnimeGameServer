#include "network/dispatcher/MessageDispatcher.h"
#include "common/logger/Logger.h"
#include "common/thread/GlobalThreadPool.h"
#include "network/Connection.h"
#include "network/protocol/MessageId.h"

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

    std::string payload(data, len);

    // 🔥 防御：非登录包必须有 session
    if (msgId != MSG_C2S_LOGIN)
    {
        if (conn->GetSessionId() == 0)
        {
            LOG_WARN("drop msg={} no session conn={}", msgId, conn->GetConnectionId());
            return;
        }
    }

    handler(conn, data, len);
}