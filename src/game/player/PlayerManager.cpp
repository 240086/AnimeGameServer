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

bool PlayerManager::Login(uint64_t uid, std::shared_ptr<Player> newPlayer)
{
    if (!newPlayer)
        return false;

    size_t idx = GetBucketIndex(uid);
    std::lock_guard<std::mutex> lock(buckets_[idx].mutex);

    auto &map = buckets_[idx].players;
    auto it = map.find(uid);
    if (it != map.end())
    {
        // 🔥 工业级处理：踢掉老玩家连接，清理旧实例
        LOG_INFO("Player {} re-login, kicking old instance", uid);
        // it->second->Kick("Multi-login"); // 假设 Player 有 Kick 方法
        map.erase(it);
    }

    map[uid] = newPlayer;
    return true;
}

std::shared_ptr<Player> PlayerManager::GetPlayer(uint64_t uid)
{
    size_t idx = GetBucketIndex(uid);
    std::lock_guard<std::mutex> lock(buckets_[idx].mutex);

    auto &map = buckets_[idx].players;
    auto it = map.find(uid);
    return (it != map.end()) ? it->second : nullptr;
}

void PlayerManager::Logout(uint64_t uid)
{
    std::shared_ptr<Player> player = nullptr;

    // 1. 从内存移除
    size_t idx = GetBucketIndex(uid);
    {
        std::lock_guard<std::mutex> lock(buckets_[idx].mutex);
        auto it = buckets_[idx].players.find(uid);
        if (it != buckets_[idx].players.end())
        {
            player = it->second;
            buckets_[idx].players.erase(it);
        }
    }

    // 2. 登出强制存库
    if (player)
    {
        LOG_INFO("Player {} logout, triggering final save", uid);
        // 登出时 forceAll = true，确保所有内存状态刷回 Redis/DB
        AsyncSavePlayer(player, true);
    }
}

void PlayerManager::AsyncSavePlayer(std::shared_ptr<Player> player, bool forceAll)
{
    if (!player)
        return;

    // 状态机保护：防止多个保存任务同时排队
    if (!player->TryMarkSaving())
        return;

    // 获取脏标记。如果是强制保存，则加上 ALL 标记
    uint32_t flags = player->FetchDirtyFlags();
    if (forceAll)
    {
        flags |= static_cast<uint32_t>(PlayerDirtyFlag::ALL);
    }

    if (flags == 0)
    {
        player->ClearSaving();
        return;
    }

    auto task = std::make_unique<SavePlayerTask>(player, flags);
    SaveQueue::Instance().Push(player->GetId(), std::move(task));
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
    // 使用原子操作增加计数，确保多线程调用安全（虽然目前通常在主线程 Tick）
    uint64_t currentTick = autosave_counter_.fetch_add(1, std::memory_order_relaxed);
    size_t bucketIndex = currentTick % BUCKET_COUNT;

    std::vector<std::shared_ptr<Player>> targets;
    {
        std::lock_guard<std::mutex> lock(buckets_[bucketIndex].mutex);
        for (auto &pair : buckets_[bucketIndex].players)
        {
            if (pair.second->IsDirty() && !pair.second->IsSaving())
            {
                targets.push_back(pair.second);
            }
        }
    }

    size_t count = 0;
    for (auto &player : targets)
    {
        if (count >= 100)
            break; // 每 Tick 最多处理 100 个，防止 IO 队列暴涨
        AsyncSavePlayer(player);
        count++;
    }
}
