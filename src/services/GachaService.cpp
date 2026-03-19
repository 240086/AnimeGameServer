#include "services/GachaService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/Packet.h"
#include "game/gacha/GachaSystem.h"
#include "common/logger/Logger.h"
#include "network/protocol/ProtoMessage.h"
#include "network/protocol/ErrorSender.h"
#include "gacha.pb.h"

namespace
{
    constexpr int kSingleDrawCost = 160;
    constexpr int kTenDrawCount = 10;

    // 辅助解析函数：封装类型转换逻辑
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
    // 注册单抽：使用 std::move 优化智能指针传递
    MessageDispatcher::Instance().RegisterHandler(MSG_C2S_GACHA_DRAW,
                                                  [this](Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
                                                  {
                                                      HandleGacha(conn, player, std::move(msg));
                                                  });

    // 注册十连抽
    MessageDispatcher::Instance().RegisterHandler(MSG_C2S_GACHA_DRAW_TEN,
                                                  [this](Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
                                                  {
                                                      HandleGachaTen(conn, player, std::move(msg));
                                                  });
}

void GachaService::HandleGacha(Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
{
    if (!conn || !player)
        return;

    auto realMsg = ParseGachaReq(msg);
    if (!realMsg)
    {
        ErrorSender::Send(conn, ErrorCode::INVALID_REQUEST, "GachaRequest parse failed");
        return;
    }

    const auto &req = realMsg->Get();
    // 提取 pool_id，默认为 1
    int poolId = req.pool_id() > 0 ? static_cast<int>(req.pool_id()) : 1;

    // 扣费逻辑
    if (!player->GetCurrency().Spend(kSingleDrawCost))
    {
        ErrorSender::Send(conn, ErrorCode::INSUFFICIENT_FUNDS);
        return;
    }

    // 执行单抽
    auto item = GachaSystem::Instance().DrawOnce(*player, poolId);
    player->GetInventory().AddItem(item.id);

    // 构造响应包
    anime::GachaDrawResponse respPb;
    respPb.set_item_id(item.id);
    respPb.set_rarity(item.rarity);

    std::string payload;
    if (!respPb.SerializeToString(&payload))
    {
        ErrorSender::Send(conn, ErrorCode::INSUFFICIENT_FUNDS);
        return;
    }

    Packet pkt;
    pkt.SetMessageId(MSG_S2C_GACHA_DRAW_RESP);
    pkt.Append(payload);
    conn->SendPacket(pkt);
}

void GachaService::HandleGachaTen(Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
{
    if (!conn || !player)
        return;

    auto realMsg = ParseGachaReq(msg);
    if (!realMsg)
    {
        LOG_ERROR("GachaService: message type mismatch for ten draw");
        return;
    }

    const auto &req = realMsg->Get();
    int poolId = req.pool_id() > 0 ? static_cast<int>(req.pool_id()) : 1;

    // 计算总消耗
    const int totalCost = kSingleDrawCost * kTenDrawCount;
    if (!player->GetCurrency().Spend(totalCost))
    {
        Packet errPkt;
        errPkt.SetMessageId(MSG_S2C_ERROR_INSUFFICIENT_FUNDS);
        conn->SendPacket(errPkt);
        return;
    }

    // 💡 核心改动：调用 GachaSystem 的批量抽取接口
    auto results = GachaSystem::Instance().DrawTen(*player, poolId);

    anime::GachaDrawTenResponse respPb;
    for (const auto &item : results)
    {
        // 更新玩家内存数据
        player->GetInventory().AddItem(item.id);

        // 填充 PB 列表
        auto *result = respPb.add_results();
        result->set_item_id(item.id);
        result->set_rarity(item.rarity);
    }

    std::string payload;
    if (respPb.SerializeToString(&payload))
    {
        Packet pkt;
        pkt.SetMessageId(MSG_S2C_GACHA_DRAW_TEN_RESP);
        pkt.Append(payload);
        conn->SendPacket(pkt);
    }

    // 打印带有 trace_id 的日志，方便追踪客户端请求
    LOG_INFO("Player {} completed 10-draw, pool_id={}, trace_id={}",
             player->GetId(), poolId, req.client_trace_id());
}