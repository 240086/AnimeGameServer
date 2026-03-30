#pragma once

#include <memory>
#include <chrono>
#include "game/actor/PlayerActor.h"

class Connection;
class Player;

class Session
{
public:
    explicit Session(uint32_t sessionId);

    uint32_t GetSessionId() const;

    void BindConnection(std::shared_ptr<Connection> conn);

    void BindPlayer(std::shared_ptr<Player> player);

    std::shared_ptr<Player> GetPlayer();

    std::shared_ptr<Connection> GetConnection();

    void UpdateHeartbeat();

    std::chrono::steady_clock::time_point GetLastHeartbeat() const;

    void BindActor(std::shared_ptr<PlayerActor> actor)
    {
        actor_ = actor;
    }

    std::shared_ptr<PlayerActor> GetActor()
    {
        return actor_;
    }

    void UnbindPlayer()
    {
        player_.reset();
    }

    void UnbindActor()
    {
        actor_.reset();
    }

private:
    uint32_t session_id_;

    std::weak_ptr<Connection> connection_;

    std::shared_ptr<Player> player_;

    std::chrono::steady_clock::time_point last_heartbeat_;

    std::shared_ptr<PlayerActor> actor_;
};