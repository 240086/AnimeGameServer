#include "services/GachaService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/Packet.h"
#include "network/session/SessionManager.h"
#include "game/gacha/GachaSystem.h"
#include "common/logger/Logger.h"
#include "network/protocol/generated/gacha.pb.h"
#include "game/player/Player.h"

// 前提建议：确保你的 Connection 类继承了 std::enable_shared_from_this<Connection>
// 这样才能在 IO 线程安全地获取 shared_ptr

GachaService &GachaService::Instance()
{
    static GachaService instance;
    return instance;
}

void GachaService::Init()
{
    // 注册单抽
    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_GACHA_DRAW,
        [this](Connection *conn, const char *data, size_t len)
        {
            HandleGacha(conn, data, len);
        });

    // 注册十连抽
    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_GACHA_DRAW_TEN,
        [this](Connection *conn, const char *data, size_t len)
        {
            HandleGachaTen(conn, data, len);
        });
}

void GachaService::HandleGacha(Connection *conn, const char *data, size_t len)
{
    if (!conn)
        return;

    // 1. [IO线程] 获取 Session 和对应的 Actor/Player
    auto session = SessionManager::Instance().GetSession(conn->GetSessionId());
    if (!session || !session->GetActor() || !session->GetPlayer())
    {
        LOG_ERROR("Session/Actor/Player context invalid for sessionId: {}", conn->GetSessionId());
        return;
    }

    // 2. [IO线程] 解析 Protobuf (卸载逻辑线程压力)
    anime::GachaDrawRequest req;
    if (!req.ParseFromArray(data, (int)len))
    {
        LOG_ERROR("GachaDrawRequest parse failed");
        return;
    }

    // 3. [关键优化] 获取 Connection 的弱引用，避免在 Actor 线程中访问 SessionManager 锁
    // 注意：这要求 Connection 是由 shared_ptr 管理的
    std::weak_ptr<Connection> weakConn = conn->shared_from_this();
    auto actor = session->GetActor();
    uint64_t playerId = session->GetPlayer()->GetId();

    // 4. [逻辑投递] 调度到 PlayerActor 串行队列
    actor->Post([actor, playerId, weakConn]()
                {
        // --- Actor Worker 线程执行：单线程安全 ---
        auto player = actor->GetPlayer();
        if (!player) return;

        // 建议：COST 应该由配置表或 GachaSystem 提供，此处为简化逻辑
        const int COST = 160;

        // 逻辑处理：扣费
        if (!player->GetCurrency().Spend(COST)) {
            LOG_WARN("Player {} insufficient currency", playerId);
            // 回路发送：锁住弱引用
            if (auto currentConn = weakConn.lock()) {
                Packet errPkt;
                errPkt.SetMessageId(MSG_S2C_ERROR_INSUFFICIENT_FUNDS);
                currentConn->SendPacket(errPkt);
            }
            return;
        }

        // 逻辑处理：抽卡计算与数据写入
        auto item = GachaSystem::Instance().DrawOnce(*player);
        player->GetInventory().AddItem(item.id);
        player->GetGachaHistory().Record(item.rarity);

        // 构造响应包
        anime::GachaDrawResponse resp_pb;
        resp_pb.set_item_id(item.id);
        resp_pb.set_rarity(item.rarity);

        std::string payload;
        if (resp_pb.SerializeToString(&payload)) {
            Packet pkt;
            pkt.SetMessageId(MSG_S2C_GACHA_DRAW_RESP);
            pkt.Append(payload.data(), payload.size());

            // 5. [回路投递] 安全发送
            if (auto currentConn = weakConn.lock()) {
                currentConn->SendPacket(pkt);
            } else {
                LOG_DEBUG("Player {} disconnected during Gacha, packet dropped.", playerId);
            }
        } });
}

void GachaService::HandleGachaTen(Connection *conn, const char *data, size_t len)
{
    if (!conn)
        return;

    auto session = SessionManager::Instance().GetSession(conn->GetSessionId());
    if (!session || !session->GetActor() || !session->GetPlayer())
        return;

    std::weak_ptr<Connection> weakConn = conn->shared_from_this();
    auto actor = session->GetActor();
    uint64_t playerId = session->GetPlayer()->GetId();

    actor->Post([actor, playerId, weakConn]()
                {
        auto player = actor->GetPlayer();
        if (!player) return;

        const int TOTAL_COST = 1600;
        if (!player->GetCurrency().Spend(TOTAL_COST)) {
            if (auto currentConn = weakConn.lock()) {
                Packet errPkt;
                errPkt.SetMessageId(MSG_S2C_ERROR_INSUFFICIENT_FUNDS);
                currentConn->SendPacket(errPkt);
            }
            return;
        }

        anime::GachaDrawTenResponse resp_pb;
        for (int i = 0; i < 10; ++i) {
            auto item = GachaSystem::Instance().DrawOnce(*player);
            player->GetInventory().AddItem(item.id);
            player->GetGachaHistory().Record(item.rarity);

            auto* result = resp_pb.add_results();
            result->set_item_id(item.id);
            result->set_rarity(item.rarity);
        }

        std::string payload;
        if (resp_pb.SerializeToString(&payload)) {
            Packet pkt;
            pkt.SetMessageId(MSG_S2C_GACHA_DRAW_TEN_RESP);
            pkt.Append(payload);

            if (auto currentConn = weakConn.lock()) {
                currentConn->SendPacket(pkt);
            }
        }
        
        LOG_INFO("Player {} completed 10-draw", playerId); });
}