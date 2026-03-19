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
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[msgId] = handler;
}

void MessageDispatcher::Dispatch(uint16_t msgId, Connection *conn, const char *data, size_t len)
{
    if (!conn)
        return;

    MessageHandler handler;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(msgId);
        if (it == handlers_.end())
        {
            LOG_ERROR("No handler for msgId: {}", msgId);
            return;
        }
        handler = it->second;
    }

    // 🔥 Step 1: 解码（新增）
    auto msg = MessageDecoder::Instance().Decode(msgId, data, len);
    if (!msg)
    {
        LOG_ERROR("Decode failed msgId={}", msgId);
        ErrorSender::Send(conn, ErrorCode::DECODE_FAILED);
        return;
    }

    std::shared_ptr<IMessage> sharedMsg = std::move(msg);
    // --- 登录协议：仍然直走 ---
    if (msgId == MSG_C2S_LOGIN)
    {
        // 登录时 Player 尚未创建，传 nullptr 是符合逻辑的
        handler(conn, nullptr, sharedMsg);
        return;
    }

    auto connPtr = conn->shared_from_this();
    uint64_t connSid = conn->GetSessionId();

    if (connSid == 0)
        return;

    auto session = SessionManager::Instance().GetSession(connSid);
    if (!session)
        return;

    auto player = session->GetPlayer();
    if (!player)
        return;

    auto actor = session->GetActor();
    if (!actor)
        return;

    // 🔥 Actor 投递（无 SessionManager 二次访问）
    actor->Post([handler,
                 connPtr,
                 connSid,
                 player,
                 sharedMsg]() mutable
                {
        if (player->GetSessionId() != connSid)
            return;

        handler(connPtr.get(), player.get(), sharedMsg); });
}