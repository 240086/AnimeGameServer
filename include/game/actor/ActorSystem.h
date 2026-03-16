#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

class Actor;

class ActorSystem
{
public:
    static ActorSystem &Instance();

    void Start(int threads);
    void Stop();
    void Schedule(std::shared_ptr<Actor> actor);

private:
    void Worker(int shardIndex); // Worker 现在绑定到特定的分片

private:
    // 分片结构体：强制缓存行对齐，避免伪共享 (False Sharing)
    struct alignas(64) Shard
    {
        std::mutex mutex;
        std::condition_variable cond;
        std::queue<std::shared_ptr<Actor>> ready_queue;
    };

    std::vector<Shard> shards_;
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{false};
};