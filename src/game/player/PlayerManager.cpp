#include "game/player/PlayerManager.h"

PlayerManager &PlayerManager::Instance()
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

    if (it != players_.end())
        return it->second;

    return nullptr;
}

void PlayerManager::RemovePlayer(uint64_t id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    players_.erase(id);
}
std::vector<std::shared_ptr<Player>> PlayerManager::GetAllPlayers()
{
    std::vector<std::shared_ptr<Player>> result;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto &p : players_)
            result.push_back(p.second);
    }

    return result;
}
void PlayerManager::ForEachPlayer(std::function<void(const std::shared_ptr<Player> &)> func)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &pair : players_)
    {
        func(pair.second);
    }
}