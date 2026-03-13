#include "services/LoginService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "common/logger/Logger.h"
#include "network/session/SessionManager.h"
#include "game/player/PlayerManager.h"
#include "network/protocol/Packet.h"

LoginService &LoginService::Instance()
{
    static LoginService instance;
    return instance;
}

void LoginService::Init()
{
    MessageDispatcher::Instance().RegisterHandler(
        MSG_C2S_LOGIN,
        [this](Connection *conn, const char *data, size_t len)
        {
            HandleLogin(conn, data, len);
        });
}

void LoginService::HandleLogin(Connection *conn, const char *data, size_t len)
{
    LOG_INFO("login request received size={}", len);

    auto session = SessionManager::Instance().GetSession(conn->GetSessionId());

    if (!session)
    {
        LOG_ERROR("session not found");
        return;
    }

    static std::atomic<uint64_t> next_player_id{1};

    uint64_t playerId = next_player_id++;

    auto player =
        PlayerManager::Instance().CreatePlayer(playerId);

    player->GetCurrency().Add(100000);
    session->BindPlayer(player);

    LoginResponse respData;

    respData.playerId = playerId;
    respData.currency = player->GetCurrency().Get();

    Packet resp;

    resp.SetMessageId(MSG_S2C_LOGIN_RESP);

    resp.Append((char *)&respData, sizeof(respData));

    conn->SendPacket(resp);
}