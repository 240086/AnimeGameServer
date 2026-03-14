#include "services/LoginService.h"

#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "network/protocol/Packet.h"

#include "network/session/SessionManager.h"
#include "game/player/PlayerManager.h"

#include "common/logger/Logger.h"

#include "network/protocol/generated/login.pb.h"

LoginService& LoginService::Instance()
{
    static LoginService instance;
    return instance;
}

void LoginService::Init()
{
    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_LOGIN,
        [this](Connection* conn, const char* data, size_t len)
        {
            HandleLogin(conn, data, len);
        });
}

void LoginService::HandleLogin(Connection* conn, const char* data, size_t len)
{
    LOG_INFO("login request received size={}", len);

    if (!conn)
    {
        LOG_ERROR("connection null");
        return;
    }

    auto session =
        SessionManager::Instance().GetSession(conn->GetSessionId());

    if (!session)
    {
        LOG_ERROR("session not found");
        return;
    }

    /* ---------- 解析 protobuf 请求 ---------- */

    anime::LoginRequest request;

    if (!request.ParseFromArray(data, (int)len))
    {
        LOG_ERROR("LoginRequest parse failed");
        return;
    }

    /* ---------- 创建玩家 ---------- */

    static std::atomic<uint64_t> next_player_id{1};

    uint64_t playerId = next_player_id++;

    auto player =
        PlayerManager::Instance().CreatePlayer(playerId);

    player->GetCurrency().Add(100000);

    session->BindPlayer(player);

    /* ---------- 构造 protobuf 响应 ---------- */

    anime::LoginResponse resp_pb;

    resp_pb.set_player_id(playerId);
    resp_pb.set_currency(player->GetCurrency().Get());

    std::string payload;

    if (!resp_pb.SerializeToString(&payload))
    {
        LOG_ERROR("LoginResponse serialize failed");
        return;
    }

    /* ---------- 发送 Packet ---------- */

    Packet packet;

    packet.SetMessageId(MSG_S2C_LOGIN_RESP);

    packet.Append(payload.data(), payload.size());

    conn->SendPacket(packet);

    LOG_INFO(
        "player {} login success currency {}",
        playerId,
        player->GetCurrency().Get());
}