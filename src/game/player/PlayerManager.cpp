#include "game/player/PlayerManager.h"
#include "game/player/Player.h"

PlayerManager &PlayerManager::Instance()
{
    static PlayerManager instance;
    return instance;
}

void PlayerManager::AddPlayer(std::shared_ptr<Player> player)
{
    if (!player)
        return;
    uint64_t uid = player->GetId();
    size_t idx = GetBucketIndex(uid);

    std::lock_guard<std::mutex> lock(buckets_[idx].mutex);
    buckets_[idx].players[uid] = player;
}

std::shared_ptr<Player> PlayerManager::GetPlayer(uint64_t uid)
{
    size_t idx = GetBucketIndex(uid);
    std::lock_guard<std::mutex> lock(buckets_[idx].mutex);

    auto &map = buckets_[idx].players;
    auto it = map.find(uid);
    return (it != map.end()) ? it->second : nullptr;
}

void PlayerManager::RemovePlayer(uint64_t uid)
{
    size_t idx = GetBucketIndex(uid);
    std::lock_guard<std::mutex> lock(buckets_[idx].mutex);
    buckets_[idx].players.erase(uid);
}

size_t PlayerManager::OnlineCount()
{
    size_t count = 0;
    for (size_t i = 0; i < BUCKET_COUNT; ++i)
    {
        std::lock_guard<std::mutex> lock(buckets_[i].mutex);
        count += buckets_[i].players.size();
    }
    return count;
}

void PlayerManager::ForEachPlayer(
    std::function<void(const std::shared_ptr<Player> &)> func)
{
    for (size_t i = 0; i < BUCKET_COUNT; ++i)
    {
        std::vector<std::shared_ptr<Player>> players;

        {
            std::lock_guard<std::mutex> lock(buckets_[i].mutex);

            for (auto &pair : buckets_[i].players)
            {
                players.push_back(pair.second);
            }
        }

        for (auto &p : players)
        {
            func(p);
        }
    }
}