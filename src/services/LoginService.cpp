#include "services/LoginService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "common/logger/Logger.h"
#include "network/session/SessionManager.h"

LoginService &LoginService::Instance()
{
    static LoginService instance;
    return instance;
}

void LoginService::Init()
{
    MessageDispatcher::Instance().RegisterHandler(
        MSG_LOGIN,
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
        return;

    // static uint64_t next_player_id = 1;

    // auto player = PlayerManager::Instance().CreatePlayer(next_player_id++);

    // session->BindPlayer(player);

    // LOG_INFO("player {} login success", player->GetId());
}