#include "services/GachaService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/Packet.h"
#include "game/gacha/GachaSystem.h"
#include "common/logger/Logger.h"
#include "network/protocol/generated/gacha.pb.h"
#include "network/protocol/ProtoMessage.h"

GachaService &GachaService::Instance()
{
    static GachaService instance;
    return instance;
}

void GachaService::Init()
{
    // 注册单抽
    MessageDispatcher::Instance().RegisterHandler(MSG_C2S_GACHA_DRAW,
                                                  [this](Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
                                                  {
                                                      HandleGacha(conn, player, msg);
                                                  });

    // 注册十连抽
    MessageDispatcher::Instance().RegisterHandler(MSG_C2S_GACHA_DRAW_TEN,
                                                  [this](Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
                                                  {
                                                      HandleGachaTen(conn, player, msg);
                                                  });
}

// =========================================================================
// 🔥 核心重构：现在的 Handle 函数已经是【绝对的单线程安全环境】
// 且 conn 的生命周期已被 Dispatcher 保护，直接使用即可！
// =========================================================================

void GachaService::HandleGacha(Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
{
    auto realMsg = std::static_pointer_cast<
        ProtoMessage<anime::GachaDrawRequest>>(msg);

    if (!realMsg)
    {
        LOG_ERROR("GachaService: Message type mismatch");
        return;
    }

    const auto &req = realMsg->Get();

    // 👉 未来可以用 req.pool_id()

    const int COST = 160;

    if (!player->GetCurrency().Spend(COST))
    {
        Packet errPkt;
        errPkt.SetMessageId(MSG_S2C_ERROR_INSUFFICIENT_FUNDS);
        conn->SendPacket(errPkt);
        return;
    }

    auto item = GachaSystem::Instance().DrawOnce(*player);

    player->GetInventory().AddItem(item.id);
    player->GetGachaHistory().Record(item.rarity);

    anime::GachaDrawResponse resp_pb;
    resp_pb.set_item_id(item.id);
    resp_pb.set_rarity(item.rarity);

    std::string payload;
    if (resp_pb.SerializeToString(&payload))
    {
        Packet pkt;
        pkt.SetMessageId(MSG_S2C_GACHA_DRAW_RESP);
        pkt.Append(payload);
        conn->SendPacket(pkt);
    }
}

void GachaService::HandleGachaTen(Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
{
    // 1. 强制类型转换：从 IMessage 找回具体的协议数据
    // 假设你的十连抽请求 PB 协议名为 anime::GachaDrawTenRequest
    auto realMsg = std::static_pointer_cast<ProtoMessage<anime::GachaDrawRequest>>(msg);
    if (!realMsg)
    {
        LOG_ERROR("GachaService: Message type mismatch, expected GachaDrawTenRequest");
        return;
    }

    const auto &req = realMsg->Get();
    uint32_t poolId = req.pool_id(); // 从客户端请求中获取目标卡池 ID

    // 2. 逻辑校验：获取卡池成本（避免硬编码 1600）
    // 工业级做法是通过 GachaSystem 根据 poolId 从配置表获取单价
    const int SINGLE_COST = 160;
    const int TOTAL_COST = SINGLE_COST * 10;

    // 3. 扣费逻辑：利用 Player 对象的原子操作（已由 Actor 保证单线程安全）
    if (!player->GetCurrency().Spend(TOTAL_COST))
    {
        LOG_WARN("Player {} insufficient currency for 10-draw in pool {}", player->GetId(), poolId);

        Packet errPkt;
        errPkt.SetMessageId(MSG_S2C_ERROR_INSUFFICIENT_FUNDS);
        // Dispatcher 已通过 shared_ptr 保证 conn 在此函数执行期间存活
        conn->SendPacket(errPkt);
        return;
    }

    // 4. 业务逻辑处理：执行 10 次抽卡
    anime::GachaDrawTenResponse resp_pb;
    for (int i = 0; i < 10; ++i)
    {
        // 传入 poolId 确保概率模型正确
        auto item = GachaSystem::Instance().DrawOnce(*player);

        // 写入玩家内存数据（Inventory 和 GachaHistory）
        player->GetInventory().AddItem(item.id);
        player->GetGachaHistory().Record(item.rarity);

        // 填充响应协议
        auto *result = resp_pb.add_results();
        result->set_item_id(item.id);
        result->set_rarity(item.rarity);
    }

    // 5. 序列化并回发给客户端
    std::string payload;
    if (resp_pb.SerializeToString(&payload))
    {
        Packet pkt;
        pkt.SetMessageId(MSG_S2C_GACHA_DRAW_TEN_RESP);
        pkt.Append(payload);

        // 发送回客户端
        conn->SendPacket(pkt);
    }

    LOG_INFO("Player {} completed 10-draw for Pool ID: {}", player->GetId(), poolId);
}