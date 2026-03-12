#include "game/player/PlayerManager.h"

PlayerManager& PlayerManager::Instance()
{
    static PlayerManager instance;
    return instance;
}

std::shared_ptr<Player> PlayerManager::CreatePlayer(uint64_t id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto player = std::make_shared<Player>(id);

    players_[id] = player;

    return player;
}

std::shared_ptr<Player> PlayerManager::GetPlayer(uint64_t id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = players_.find(id);

    if(it != players_.end())
        return it->second;

    return nullptr;
}

void PlayerManager::RemovePlayer(uint64_t id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    players_.erase(id);
}