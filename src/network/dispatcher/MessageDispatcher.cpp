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

void MessageDispatcher::Dispatch(MessageContext ctx, const char *data, size_t len)
{
    // 🔥 防御 1：立刻快照，防止 ctx 在后续解码中被内存踩踏破坏
    const uint32_t raw_sid = ctx.sid;

    // 解码（如果这里有内存溢出，踩的是原始 ctx）
    auto msg = MessageDecoder::Instance().Decode(ctx.msgId, data, len);
    if (!msg)
    {
        LOG_ERROR("Decode failed msgId={}, sid={}, seq={}, len={}", ctx.msgId, ctx.sid, ctx.seqId, len);
        return;
    }

    // 🔥 防御 2：强制恢复可能被踩坏的 sid
    ctx.sid = raw_sid;

    // 🔥 防御 3：在分发前尝试寻找 Session
    // 这样即便登录包还没完成，如果有旧 Session 也能被挂载
    if (!ctx.session)
    {
        ctx.session = SessionManager::Instance().GetSession(ctx.sid);
    }

    MessageHandler handler;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = handlers_.find(ctx.msgId);
        if (it != handlers_.end())
            handler = it->second;
    }

    if (!handler)
        return;

    auto actor = ctx.session ? ctx.session->GetActor() : nullptr;

    if (actor)
    {
        // 既然已经有 Actor 了，为了绝对安全，强引用存一份
        auto sessionPtr = ctx.session;
        actor->Post([handler, ctx, msg, sessionPtr]() mutable
                    { handler(ctx, msg); });
    }
    else
    {
        // 登录/注册消息走这里
        // 建议：哪怕是原地执行，也确保 ctx 是最新的快照
        handler(ctx, msg);
    }
}