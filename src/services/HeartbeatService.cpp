#include "services/HeartbeatService.h"

#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/session/SessionManager.h"
#include "network/protocol/Packet.h"
#include "network/protocol/ProtoMessage.h"
#include "heartbeat.pb.h" // 💡 统一使用生成的 PB 头文件

#include "common/logger/Logger.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include <ctime> // 💡 引入 time() 所需头文件

HeartbeatService &HeartbeatService::Instance()
{
    static HeartbeatService instance;
    return instance;
}

void HeartbeatService::Init()
{
    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_HEARTBEAT,
        [this](Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
        {
            // 💡 建议此处也可以像 GachaService 一样使用 std::move(msg)
            HandleHeartbeat(conn, player, std::move(msg));
        });
}

void HeartbeatService::HandleHeartbeat(
    Connection *conn, Player *player, std::shared_ptr<IMessage> msg)
{
    if (!conn)
        return;

    // 🔥 核心重构：审计并校验 IMessage 类型
    // 即便心跳包目前没有字段，也必须确保它是一个有效的 HeartbeatRequest
    auto heartbeatMsg = std::dynamic_pointer_cast<ProtoMessage<anime::HeartbeatRequest>>(msg);
    if (!heartbeatMsg)
    {
        LOG_ERROR("HeartbeatService: message type mismatch");
        return;
    }

    // 💡 显式忽略未使用的参数，防止编译器警告
    (void)player;

    auto session = SessionManager::Instance().GetSession(conn->GetSessionId());
    if (!session)
    {
        LOG_WARN("heartbeat session not found");
        return;
    }

    // 更新心跳时间戳，防止被 SessionManager 的超时检测踢掉
    session->UpdateHeartbeat();

    // 构造响应
    anime::HeartbeatResponse resp;
    resp.set_server_time(time(nullptr)); // 返回服务器当前 UNIX 时间戳

    std::string payload;
    if (!resp.SerializeToString(&payload))
    {
        LOG_ERROR("HeartbeatResponse serialize failed");
        return;
    }

    Packet packet;
    packet.SetMessageId(MSG_S2C_HEARTBEAT_RESP);
    packet.Append(payload.data(), payload.size());

    // 安全回发心跳响应
    conn->SendPacket(packet);
}