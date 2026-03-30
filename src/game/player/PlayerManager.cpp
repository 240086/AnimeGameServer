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
        LOG_DEBUG("Player {} re-login, kicking old instance", uid);
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
    // 1. 第一步：先从容器中获取该玩家（此时不移除，只读）
    // 这里使用分桶锁保证 Get 操作的线程安全
    auto player = GetPlayer(uid);
    if (!player)
    {
        return; // 玩家已经不在内存中，直接返回
    }

    // 2. 第二步：🔥 CAS 原子操作抢占登出权
    // 即使 CheckTimeout、Session 关闭、顶号逻辑同时调用 Logout
    // 只有第一个线程能通过此检查，后续线程会在这里被拦截
    if (!player->TryMarkLoggingOut())
    {
        LOG_WARN("Player {} is already in logout sequence, skipping.", uid);
        return;
    }

    // 3. 第三步：从内存容器中正式移除
    // 一旦执行此步，后续的 GetPlayer 将无法再搜到此实例
    size_t idx = GetBucketIndex(uid);
    {
        std::lock_guard<std::mutex> lock(buckets_[idx].mutex);
        buckets_[idx].players.erase(uid);
    }

    // 4. 第四步：执行最终存盘
    // 此时 player 依然被当前函数持有一个 shared_ptr 引用，保证对象存活
    LOG_DEBUG("Player {} logout initiated, triggering final mandatory save.", uid);

    // 登出场景优先依赖脏标记增量存盘，避免压测/短连接场景下每次下线都触发全量写库
    // 如需强一致全量落盘，建议仅在关服流程显式触发 forceAll
    AsyncSavePlayer(player, false);
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
