#include "services/HeartbeatService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/MessageContext.h"
#include "network/protocol/ProtoMessage.h"
#include "network/protocol/ResponseSender.h"
#include "network/session/SessionManager.h"
#include "common/logger/Logger.h"
#include "heartbeat.pb.h"
#include <ctime>

HeartbeatService &HeartbeatService::Instance()
{
    static HeartbeatService instance;
    return instance;
}

void HeartbeatService::Init()
{
    // 🔥 审计：统一使用 MessageContext 签名
    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_HEARTBEAT,
        [this](const MessageContext &ctx, std::shared_ptr<anime::IMessage> msg)
        {
            this->HandleHeartbeat(ctx, std::move(msg));
        });
}

void HeartbeatService::HandleHeartbeat(const MessageContext &ctx, std::shared_ptr<anime::IMessage> msg)
{
    // 1. 基础校验
    if (!ctx.conn)
        return;

    // 2. 校验消息类型（即使 Request 为空，校验类型也是防御性编程的好习惯）
    auto heartbeatMsg = std::dynamic_pointer_cast<ProtoMessage<anime::HeartbeatRequest>>(msg);
    if (!heartbeatMsg)
    {
        LOG_ERROR("Heartbeat type mismatch, sid={}", ctx.sid);
        return;
    }

    // 3. 更新 Session 心跳时间
    // 💡 优化：优先使用 ctx 携带的 session，避免去 Manager 再次查找
    auto session = ctx.session;
    if (session)
    {
        session->UpdateHeartbeat();
    }
    else
    {
        // 如果是登录前的连接心跳，可以根据 sid 查找或忽略
        auto s = SessionManager::Instance().GetSession(ctx.sid);
        if (s)
            s->UpdateHeartbeat();
    }

    // 4. 构造 PB 响应
    anime::HeartbeatResponse resp;
    resp.set_server_time(static_cast<uint64_t>(time(nullptr)));

    // 5. 🔥 核心修复：使用 ResponseSender 发送
    // 这样会自动处理 ctx.sid 和 ctx.seqId，确保网关能精准回传给客户端
    ResponseSender::Send(ctx, MSG_S2C_HEARTBEAT_RESP, resp);

    // 性能审计：心跳通常不记录 INFO 日志，仅在 DEBUG 开启
    LOG_DEBUG("Heartbeat handled for sid={}", ctx.sid);
}