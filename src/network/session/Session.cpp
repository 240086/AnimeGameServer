#include "network/session/Session.h"
#include "network/Connection.h"
#include "game/player/Player.h"

Session::Session(uint64_t sessionId)
    : session_id_(sessionId)
{
    last_heartbeat_ = std::chrono::steady_clock::now();
}

uint64_t Session::GetSessionId() const
{
    return session_id_;
}

void Session::BindConnection(std::shared_ptr<Connection> conn)
{
    connection_ = conn;
}

void Session::BindPlayer(std::shared_ptr<Player> player)
{
    player_ = player;
}

std::shared_ptr<Player> Session::GetPlayer()
{
    return player_;
}

std::shared_ptr<Connection> Session::GetConnection()
{
    return connection_.lock();
}

void Session::UpdateHeartbeat()
{
    last_heartbeat_ = std::chrono::steady_clock::now();
}

std::chrono::steady_clock::time_point Session::GetLastHeartbeat() const
{
    return last_heartbeat_;
}