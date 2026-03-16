#include "game/player/PlayerManager.h"
#include "game/player/Player.h"
#include "database/queue/SaveQueue.h"
#include "database/task/SavePlayerTask.h"
#include "common/logger/Logger.h"

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

void PlayerManager::OnAutoSaveTick()
{
    autosave_counter_++;

    size_t bucketIndex = autosave_counter_ % BUCKET_COUNT;

    std::vector<std::shared_ptr<Player>> dirtyPlayers;

    {
        std::lock_guard<std::mutex> lock(buckets_[bucketIndex].mutex);

        for (auto &pair : buckets_[bucketIndex].players)
        {
            if (pair.second->IsDirty())
            {
                dirtyPlayers.push_back(pair.second);
            }
        }
    }

    size_t saveCount = 0;
    const size_t MAX_SAVE_PER_TICK = 100;

    for (auto &player : dirtyPlayers)
    {
        if (saveCount >= MAX_SAVE_PER_TICK)
            break;

        uint32_t flags = player->FetchDirtyFlags();
        if (flags == 0)
            continue;

        auto task = std::make_unique<SavePlayerTask>(player, flags);
        SaveQueue::Instance().Push(player->GetId(), std::move(task));

        saveCount++;
    }
}

void PlayerManager::RemovePlayerWithSave(uint64_t uid)
{
    size_t idx = GetBucketIndex(uid);
    std::shared_ptr<Player> player = nullptr;

    {
        std::lock_guard<std::mutex> lock(buckets_[idx].mutex);
        auto it = buckets_[idx].players.find(uid);
        if (it != buckets_[idx].players.end())
        {
            player = it->second;
            buckets_[idx].players.erase(it);
        }
    }

    // 如果玩家存在，执行最后一次全量存盘（强制所有位）
    if (player)
    {
        // 下线存盘通常建议覆盖所有标志位，确保万无一失
        uint32_t flags = player->FetchDirtyFlags() | static_cast<uint32_t>(PlayerDirtyFlag::ALL);
        auto task = std::make_unique<SavePlayerTask>(player, flags);
        SaveQueue::Instance().Push(player->GetId(), std::move(task));

        LOG_INFO("Player {} removed and final save task pushed.", uid);
    }
}