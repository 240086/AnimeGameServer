#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>

class DatabaseTask;

class SaveQueue
{
public:
    static SaveQueue &Instance();

    void Push(uint64_t playerId, std::unique_ptr<DatabaseTask> task);

    std::unique_ptr<DatabaseTask> Pop(size_t shardIndex);

    size_t GetShardCount() const;

private:
    static constexpr size_t SHARD_COUNT = 16;

    struct Shard
    {
        alignas(64) // 强制缓存行对齐（通常为 64 字节）
            std::mutex mutex;
        std::condition_variable cond;
        std::queue<std::unique_ptr<DatabaseTask>> queue;
    };

    std::vector<std::unique_ptr<Shard>> shards_;

    SaveQueue();
};