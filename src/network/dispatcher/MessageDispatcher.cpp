#include "network/dispatcher/MessageDispatcher.h"
#include "common/logger/Logger.h"
#include "common/thread/GlobalThreadPool.h"
#include "network/Connection.h"
#include "network/protocol/MessageId.h"
#include "network/session/SessionManager.h"
#include "network/protocol/MessageDecoder.h"
#include "game/player/Player.h"
#include "network/protocol/ErrorSender.h"

MessageDispatcher &MessageDispatcher::Instance()
{
    static MessageDispatcher instance;
    return instance;
}

void MessageDispatcher::RegisterHandler(uint16_t msgId, MessageHandler handler)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    handlers_[msgId] = std::move(handler);
}

void MessageDispatcher::Dispatch(const MessageContext &ctx,
                                 const char *data,
                                 size_t len)
{
    MessageHandler handler;

    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = handlers_.find(ctx.msgId);
        if (it == handlers_.end())
        {
            LOG_ERROR("No handler for msgId: {}", ctx.msgId);
            return;
        }
        handler = it->second;
    }

    // 解码
    auto msg = MessageDecoder::Instance().Decode(ctx.msgId, data, len);
    if (!msg)
    {
        LOG_ERROR("Decode failed msgId={}", ctx.msgId);
        return;
    }

    // Actor 投递（关键）
    auto actor = ctx.session ? ctx.session->GetActor() : nullptr;

    if (actor)
    {
        actor->Post([handler = std::move(handler), ctx, msg = std::move(msg)]() mutable
                    { handler(ctx, msg); });
    }
    else
    {
        // 鉴权类消息（Login/Register）直接原地执行或交给业务线程池
        handler(ctx, msg);
    }
}