#pragma once

#include <memory>
#include <chrono>

class Connection;
class Player;

class Session
{
public:

    explicit Session(uint64_t sessionId);

    uint64_t GetSessionId() const;

    void BindConnection(std::shared_ptr<Connection> conn);

    void BindPlayer(std::shared_ptr<Player> player);

    std::shared_ptr<Player> GetPlayer();

    std::shared_ptr<Connection> GetConnection();

    void UpdateHeartbeat();

    std::chrono::steady_clock::time_point GetLastHeartbeat() const;

private:

    uint64_t session_id_;

    std::shared_ptr<Connection> connection_;

    std::shared_ptr<Player> player_;

    std::chrono::steady_clock::time_point last_heartbeat_;
};