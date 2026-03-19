#include "services/GachaService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/Packet.h"
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

    std::shared_ptr<ProtoMessage<anime::GachaDrawRequest>> ParseGachaReq(const std::shared_ptr<IMessage> &msg)
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
    MessageDispatcher::Instance().RegisterHandler(MSG_C2S_GACHA_DRAW,
                                                  [this](Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
                                                  {
                                                      HandleGacha(conn, player, std::move(msg));
                                                  });

    MessageDispatcher::Instance().RegisterHandler(MSG_C2S_GACHA_DRAW_TEN,
                                                  [this](Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
                                                  {
                                                      HandleGachaTen(conn, player, std::move(msg));
                                                  });
}

//////////////////////////////////////////////////////////////
// 单抽
//////////////////////////////////////////////////////////////
void GachaService::HandleGacha(Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
{
    if (!conn || !player)
        return;

    ServerMetrics::Instance().IncGachaRequest();
    ServerMetrics::Instance().IncGachaSingleRequest();

    auto realMsg = ParseGachaReq(msg);
    if (!realMsg)
    {
        ServerMetrics::Instance().IncGachaInvalidRequest();
        ErrorSender::Send(conn, ErrorCode::INVALID_REQUEST, "GachaRequest parse failed");
        return;
    }

    const auto &req = realMsg->Get();
    int poolId = req.pool_id() > 0 ? static_cast<int>(req.pool_id()) : 1;
    const std::string &traceId = req.client_trace_id();

    // ================================
    // 1. 幂等入口（核心）
    // ================================
    auto idem = IdempotencyService::Instance().CheckAndLock(player->GetId(), traceId);

    if (idem.state == IdempotencyState::IN_PROGRESS)
    {
        ServerMetrics::Instance().IncGachaInvalidRequest();
        ErrorSender::Send(conn, ErrorCode::INVALID_REQUEST, "Request in progress");
        return;
    }

    if (idem.state == IdempotencyState::COMPLETED)
    {
        ServerMetrics::Instance().IncGachaSuccess();
        ResponseSender::SendPayload(conn, MSG_S2C_GACHA_DRAW_RESP, *idem.payload);
        return;
    }

    // ================================
    // 2. 扣费校验
    // ================================
    if (!player->GetCurrency().Spend(kSingleDrawCost))
    {
        ServerMetrics::Instance().IncGachaInsufficientFunds();
        IdempotencyService::Instance().Unlock(player->GetId(), traceId);
        ErrorSender::Send(conn, ErrorCode::INSUFFICIENT_FUNDS);
        return;
    }

    // ================================
    // 3. 核心逻辑
    // ================================
    auto item = GachaSystem::Instance().DrawOnce(*player, poolId);
    player->GetInventory().AddItem(item.id);

    // ================================
    // 4. 构建响应
    // ================================
    anime::GachaDrawResponse respPb;
    respPb.set_item_id(item.id);
    respPb.set_rarity(item.rarity);

    std::string payload;
    if (!respPb.SerializeToString(&payload))
    {
        ServerMetrics::Instance().IncGachaInternalError();
        IdempotencyService::Instance().Unlock(player->GetId(), traceId);
        ErrorSender::Send(conn, ErrorCode::INTERNAL_ERROR);
        return;
    }

    // ================================
    // 5. 标脏（必须在成功后）
    // ================================
    player->MarkDirty(PlayerDirtyFlag::CURRENCY);
    player->MarkDirty(PlayerDirtyFlag::INVENTORY);
    player->MarkDirty(PlayerDirtyFlag::GACHA_HISTORY);

    // ================================
    // 6. 保存幂等结果
    // ================================
    IdempotencyService::Instance().SaveResult(player->GetId(), traceId, payload);

    // ================================
    // 7. 返回
    // ================================
    ResponseSender::SendPayload(conn, MSG_S2C_GACHA_DRAW_RESP, payload);
    ServerMetrics::Instance().IncGachaSuccess();
}

//////////////////////////////////////////////////////////////
// 十连
//////////////////////////////////////////////////////////////
void GachaService::HandleGachaTen(Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
{
    if (!conn || !player)
        return;

    ServerMetrics::Instance().IncGachaRequest();
    ServerMetrics::Instance().IncGachaTenRequest();

    auto realMsg = ParseGachaReq(msg);
    if (!realMsg)
    {
        ServerMetrics::Instance().IncGachaInvalidRequest();
        ErrorSender::Send(conn, ErrorCode::INVALID_REQUEST);
        return;
    }

    const auto &req = realMsg->Get();
    int poolId = req.pool_id() > 0 ? static_cast<int>(req.pool_id()) : 1;
    const std::string &traceId = req.client_trace_id();

    // ================================
    // 1. 幂等入口
    // ================================
    auto idem = IdempotencyService::Instance().CheckAndLock(player->GetId(), traceId);

    if (idem.state == IdempotencyState::IN_PROGRESS)
    {
        ServerMetrics::Instance().IncGachaInvalidRequest();
        ErrorSender::Send(conn, ErrorCode::INVALID_REQUEST, "Request in progress");
        return;
    }

    if (idem.state == IdempotencyState::COMPLETED)
    {
        ServerMetrics::Instance().IncGachaSuccess();
        ResponseSender::SendPayload(conn, MSG_S2C_GACHA_DRAW_TEN_RESP, *idem.payload);
        return;
    }

    // ================================
    // 2. 扣费
    // ================================
    const int totalCost = kSingleDrawCost * kTenDrawCount;

    if (!player->GetCurrency().Spend(totalCost))
    {
        ServerMetrics::Instance().IncGachaInsufficientFunds();
        IdempotencyService::Instance().Unlock(player->GetId(), traceId);
        ErrorSender::Send(conn, ErrorCode::INSUFFICIENT_FUNDS);
        return;
    }

    // ================================
    // 3. 执行逻辑
    // ================================
    auto results = GachaSystem::Instance().DrawTen(*player, poolId);

    anime::GachaDrawTenResponse respPb;

    for (const auto &item : results)
    {
        player->GetInventory().AddItem(item.id);

        auto *r = respPb.add_results();
        r->set_item_id(item.id);
        r->set_rarity(item.rarity);
    }

    // ================================
    // 4. 序列化
    // ================================
    std::string payload;
    if (!respPb.SerializeToString(&payload))
    {
        ServerMetrics::Instance().IncGachaInternalError();
        IdempotencyService::Instance().Unlock(player->GetId(), traceId);
        ErrorSender::Send(conn, ErrorCode::INTERNAL_ERROR);
        return;
    }

    // ================================
    // 5. 标脏
    // ================================
    player->MarkDirty(PlayerDirtyFlag::CURRENCY);
    player->MarkDirty(PlayerDirtyFlag::INVENTORY);
    player->MarkDirty(PlayerDirtyFlag::GACHA_HISTORY);

    // ================================
    // 6. 保存结果
    // ================================
    IdempotencyService::Instance().SaveResult(player->GetId(), traceId, payload);

    // ================================
    // 7. 返回
    // ================================
    ResponseSender::SendPayload(conn, MSG_S2C_GACHA_DRAW_TEN_RESP, payload);
    ServerMetrics::Instance().IncGachaSuccess();

    LOG_INFO("Player {} completed 10-draw, pool={}, trace={}",
             player->GetId(), poolId, traceId);
}