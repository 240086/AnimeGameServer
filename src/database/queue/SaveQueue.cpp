#include "database/queue/SaveQueue.h"
#include "database/task/DatabaseTask.h"

SaveQueue::SaveQueue()
{
    // 手动初始化每个分片
    for (size_t i = 0; i < SHARD_COUNT; ++i)
    {
        shards_.push_back(std::make_unique<Shard>());
    }
}

SaveQueue &SaveQueue::Instance()
{
    static SaveQueue instance;
    return instance;
}

void SaveQueue::Push(uint64_t playerId, std::unique_ptr<DatabaseTask> task)
{
    size_t shard = playerId % SHARD_COUNT;

    auto &s = *shards_[shard];

    {
        std::lock_guard<std::mutex> lock(s.mutex);
        s.queue.push(std::move(task));
    }

    s.cond.notify_one();
}

std::unique_ptr<DatabaseTask> SaveQueue::Pop(size_t shardIndex)
{
    auto &s = *shards_[shardIndex];

    std::unique_lock<std::mutex> lock(s.mutex);

    s.cond.wait(lock, [&]
                { return !s.queue.empty(); });

    auto task = std::move(s.queue.front());
    s.queue.pop();

    return task;
}

size_t SaveQueue::GetShardCount() const
{
    return SHARD_COUNT;
}