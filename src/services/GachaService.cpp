#include "services/GachaService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "game/gacha/GachaSystem.h"
#include "common/logger/Logger.h"
#include "network/protocol/ProtoMessage.h"
#include "network/protocol/ErrorSender.h"
#include "services/IdempotencyService.h"
#include "common/metrics/ServerMetrics.h"
#include "network/protocol/ResponseSender.h"
#include "gacha.pb.h"

namespace
{
    constexpr int kSingleDrawCost = 160;
    constexpr int kTenDrawCount = 10;

    std::shared_ptr<ProtoMessage<anime::GachaDrawRequest>> ParseGachaReq(const std::shared_ptr<anime::IMessage> &msg)
    {
        return std::dynamic_pointer_cast<ProtoMessage<anime::GachaDrawRequest>>(msg);
    }
}

GachaService &GachaService::Instance()
{
    static GachaService instance;
    return instance;
}

void GachaService::Init()
{
    // 🔥 审计重点：MessageHandler 签名已更新，ctx 自动透传 sid 和 player
    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_GACHA_DRAW,
        [this](const MessageContext &ctx, std::shared_ptr<anime::IMessage> msg)
        {
            this->HandleGacha(ctx, std::move(msg));
        });

    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_GACHA_DRAW_TEN,
        [this](const MessageContext &ctx, std::shared_ptr<anime::IMessage> msg)
        {
            this->HandleGachaTen(ctx, std::move(msg));
        });
}

void GachaService::HandleGacha(const MessageContext &ctx, std::shared_ptr<anime::IMessage> msg)
{
    // 👉 统一使用 ctx 获取上下文
    auto player = ctx.player;
    if (!ctx.conn || !player)
        return;

    ServerMetrics::Instance().IncGachaRequest();
    ServerMetrics::Instance().IncGachaSingleRequest();

    auto realMsg = ParseGachaReq(msg);
    if (!realMsg)
    {
        ServerMetrics::Instance().IncGachaInvalidRequest();
        ErrorSender::Send(ctx, ErrorCode::INVALID_REQUEST, "GachaRequest parse failed");
        return;
    }

    const auto &req = realMsg->Get();
    int poolId = req.pool_id() > 0 ? static_cast<int>(req.pool_id()) : 1;
    const std::string &traceId = req.client_trace_id();

    // 1. 幂等检查：使用 player->GetId() 而非传参，确保数据一致性
    auto idem = IdempotencyService::Instance().CheckAndLock(player->GetId(), traceId);

    if (idem.state == IdempotencyState::IN_PROGRESS)
    {
        ServerMetrics::Instance().IncGachaInvalidRequest();
        ErrorSender::Send(ctx, ErrorCode::INVALID_REQUEST, "Request in progress");
        return;
    }

    if (idem.state == IdempotencyState::COMPLETED)
    {
        ServerMetrics::Instance().IncGachaSuccess();
        ResponseSender::SendPayload(ctx, MSG_S2C_GACHA_DRAW_RESP, *idem.payload);
        return;
    }

    // 2. 扣费校验：此时已在 PlayerActor 线程中，天然线程安全，无需加锁
    if (!player->GetCurrency().Spend(kSingleDrawCost))
    {
        ServerMetrics::Instance().IncGachaInsufficientFunds();
        IdempotencyService::Instance().Unlock(player->GetId(), traceId);
        ErrorSender::Send(ctx, ErrorCode::INSUFFICIENT_FUNDS);
        return;
    }

    // 3. 执行逻辑
    auto item = GachaSystem::Instance().DrawOnce(*player, poolId);
    player->GetInventory().AddItem(item.id);

    // 4. 构建响应
    anime::GachaDrawResponse respPb;
    respPb.set_item_id(item.id);
    respPb.set_rarity(item.rarity);

    std::string payload;
    if (!respPb.SerializeToString(&payload))
    {
        ServerMetrics::Instance().IncGachaInternalError();
        IdempotencyService::Instance().Unlock(player->GetId(), traceId);
        ErrorSender::Send(ctx, ErrorCode::INTERNAL_ERROR);
        return;
    }

    // 5. 标脏与保存幂等
    player->MarkDirty(PlayerDirtyFlag::CURRENCY);
    player->MarkDirty(PlayerDirtyFlag::INVENTORY);
    IdempotencyService::Instance().SaveResult(player->GetId(), traceId, payload);

    // 6. 返回结果：使用 ctx 确保网关能精准送达
    ResponseSender::SendPayload(ctx, MSG_S2C_GACHA_DRAW_RESP, payload);
    ServerMetrics::Instance().IncGachaSuccess();
}

void GachaService::HandleGachaTen(const MessageContext &ctx, std::shared_ptr<anime::IMessage> msg)
{
    auto player = ctx.player;
    if (!ctx.conn || !player)
        return;

    LOG_DEBUG("GachaTen DEBUG: ctx.sid={}, ctx.seqId={}", ctx.sid, ctx.seqId);

    ServerMetrics::Instance().IncGachaRequest();
    ServerMetrics::Instance().IncGachaTenRequest();

    auto realMsg = ParseGachaReq(msg);
    if (!realMsg)
    {
        ServerMetrics::Instance().IncGachaInvalidRequest();
        ErrorSender::Send(ctx, ErrorCode::INVALID_REQUEST);
        return;
    }

    const auto &req = realMsg->Get();
    int poolId = req.pool_id() > 0 ? static_cast<int>(req.pool_id()) : 1;
    const std::string &traceId = req.client_trace_id();

    // 1. 幂等逻辑
    auto idem = IdempotencyService::Instance().CheckAndLock(player->GetId(), traceId);
    if (idem.state == IdempotencyState::IN_PROGRESS)
    {
        ErrorSender::Send(ctx, ErrorCode::INVALID_REQUEST, "Request in progress");
        return;
    }
    if (idem.state == IdempotencyState::COMPLETED)
    {
        ResponseSender::SendPayload(ctx, MSG_S2C_GACHA_DRAW_TEN_RESP, *idem.payload);
        return;
    }

    // 2. 执行扣费与十连逻辑
    const int totalCost = kSingleDrawCost * kTenDrawCount;
    if (!player->GetCurrency().Spend(totalCost))
    {
        ServerMetrics::Instance().IncGachaInsufficientFunds();
        IdempotencyService::Instance().Unlock(player->GetId(), traceId);
        ErrorSender::Send(ctx, ErrorCode::INSUFFICIENT_FUNDS);
        return;
    }

    auto results = GachaSystem::Instance().DrawTen(*player, poolId);
    anime::GachaDrawTenResponse respPb;
    for (const auto &item : results)
    {
        player->GetInventory().AddItem(item.id);
        auto *r = respPb.add_results();
        r->set_item_id(item.id);
        r->set_rarity(item.rarity);
    }

    // 3. 序列化与返回
    std::string payload;
    if (!respPb.SerializeToString(&payload))
    {
        IdempotencyService::Instance().Unlock(player->GetId(), traceId);
        ErrorSender::Send(ctx, ErrorCode::INTERNAL_ERROR);
        return;
    }

    player->MarkDirty(PlayerDirtyFlag::CURRENCY);
    player->MarkDirty(PlayerDirtyFlag::INVENTORY);
    IdempotencyService::Instance().SaveResult(player->GetId(), traceId, payload);

    ResponseSender::SendPayload(ctx, MSG_S2C_GACHA_DRAW_TEN_RESP, payload);
    ServerMetrics::Instance().IncGachaSuccess();

    LOG_DEBUG("Player {} 10-draw success, sid={}, trace={}", player->GetId(), ctx.sid, traceId);
}