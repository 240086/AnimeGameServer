#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>

#include "game/player/Player.h"

class PlayerManager
{
public:

    static PlayerManager& Instance();

    std::shared_ptr<Player> CreatePlayer(uint64_t id);

    std::shared_ptr<Player> GetPlayer(uint64_t id);

    void RemovePlayer(uint64_t id);

private:

    std::unordered_map<uint64_t,std::shared_ptr<Player>> players_;

    std::mutex mutex_;
};