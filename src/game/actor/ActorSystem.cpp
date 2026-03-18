#include "game/actor/ActorSystem.h"
#include "game/actor/Actor.h"

ActorSystem &ActorSystem::Instance()
{
    static ActorSystem instance;
    return instance;
}

void ActorSystem::Start(int threads)
{
    running_.store(true);

    // 初始化分片，分片数量与线程数一致（严格绑定模式）
    shards_ = std::vector<Shard>(threads);

    for (int i = 0; i < threads; i++)
    {
        // 传入分片索引，让每个线程只负责自己的分片
        workers_.emplace_back(&ActorSystem::Worker, this, i);
    }
}

void ActorSystem::Stop()
{
    running_.store(false);

    // 唤醒所有分片中的阻塞线程
    for (auto &shard : shards_)
    {
        shard.cond.notify_all();
    }

    for (auto &t : workers_)
    {
        if (t.joinable())
            t.join();
    }

    workers_.clear();
    shards_.clear();
}

void ActorSystem::Schedule(std::shared_ptr<Actor> actor)
{
    if (!running_.load() || !actor)
        return;

    // 🔥 使用业务ID路由
    size_t shardIndex = actor->GetRoutingKey() % shards_.size();
    auto &shard = shards_[shardIndex];

    {
        std::lock_guard<std::mutex> lock(shard.mutex);

        // 🔥 限流（防雪崩）
        constexpr size_t MAX_QUEUE_SIZE = 10000;
        if (shard.ready_queue.size() > MAX_QUEUE_SIZE)
        {
            return; // 可以后续加日志
        }

        shard.ready_queue.push(std::move(actor));
    }

    shard.cond.notify_one();
}

void ActorSystem::Worker(int shardIndex)
{
    auto &shard = shards_[shardIndex];

    while (true)
    {
        std::shared_ptr<Actor> actor = nullptr;

        {
            std::unique_lock<std::mutex> lock(shard.mutex);
            shard.cond.wait(lock, [this, &shard]
                            { return !running_.load() || !shard.ready_queue.empty(); });

            if (!running_.load() && shard.ready_queue.empty())
                return;

            if (!shard.ready_queue.empty())
            {
                actor = std::move(shard.ready_queue.front());
                shard.ready_queue.pop();
            }
        }

        if (actor)
        {
            // 极简主义：Worker 只管触发，状态机全权交由 Actor 内部控制
            actor->Process(64);
        }
    }
}