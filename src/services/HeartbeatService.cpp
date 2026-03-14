#include "services/HeartbeatService.h"

#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/session/SessionManager.h"
#include "network/protocol/generated/heartbeat.pb.h"
#include "network/protocol/Packet.h"

#include "common/logger/Logger.h"
#include "network/Connection.h"

HeartbeatService& HeartbeatService::Instance()
{
    static HeartbeatService instance;
    return instance;
}

void HeartbeatService::Init()
{
    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_HEARTBEAT,
        [this](Connection* conn, const char* data, size_t len)
        {
            HandleHeartbeat(conn, data, len);
        });
}

void HeartbeatService::HandleHeartbeat(
    Connection* conn,
    const char* data,
    size_t len)
{
    if (!conn)
        return;

    auto session =
        SessionManager::Instance().GetSession(conn->GetSessionId());

    if (!session)
    {
        LOG_WARN("heartbeat session not found");
        return;
    }

    session->UpdateHeartbeat();

    anime::HeartbeatResponse resp;
    resp.set_server_time(time(nullptr));

    std::string payload;

    if (!resp.SerializeToString(&payload))
    {
        LOG_ERROR("HeartbeatResponse serialize failed");
        return;
    }

    Packet packet;

    packet.SetMessageId(MSG_S2C_HEARTBEAT_RESP);

    packet.Append(payload.data(), payload.size());

    conn->SendPacket(packet);
}