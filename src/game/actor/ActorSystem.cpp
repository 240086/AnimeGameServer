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
    if (!running_.load(std::memory_order_relaxed) || !actor)
        return;

    size_t shardIndex = actor->GetRoutingKey() % shards_.size();
    auto &shard = shards_[shardIndex];

    {
        std::lock_guard<std::mutex> lock(shard.mutex);

        constexpr size_t MAX_QUEUE_SIZE = 10000;

        if (shard.ready_queue.size() > MAX_QUEUE_SIZE)
        {
            // 🔴 不再修改 actor 状态
            // 这里只能丢调度请求，但 actor 仍保持 scheduled=true
            return;
        }

        shard.ready_queue.push(std::move(actor));
    }

    shard.cond.notify_one();
}

void ActorSystem::Worker(int shardIndex)
{
    auto &localShard = shards_[shardIndex];

    while (true)
    {
        std::shared_ptr<Actor> actor = nullptr;

        // 1. 优先处理本地队列
        if (localShard.TryPop(actor))
        {
            // fast path
        }
        else
        {
            // 2. 尝试偷任务（关键）
            if (!TrySteal(shardIndex, shards_, actor))
            {
                // 3. 进入等待（避免空转）
                std::unique_lock<std::mutex> lock(localShard.mutex);

                localShard.cond.wait_for(
                    lock,
                    std::chrono::milliseconds(1),
                    [this, &localShard]
                    {
                        return !running_.load() ||
                               !localShard.ready_queue.empty();
                    });

                if (!running_.load() && localShard.ready_queue.empty())
                    return;

                continue;
            }
        }

        // 4. 执行 Actor
        if (actor)
        {
            size_t pending = actor->GetMailboxSize();

            int budget = 64;
            if (pending >= 512)
                budget = 512;
            else if (pending >= 128)
                budget = 256;
            else if (pending >= 64)
                budget = 128;

            actor->Process(budget);
        }
    }
}

bool ActorSystem::TrySteal(size_t selfIndex,
                           std::vector<ActorSystem::Shard> &shards,
                           std::shared_ptr<Actor> &actor)
{
    const size_t n = shards.size();
    if (n <= 1)
        return false;

    // 使用 thread_local 随机数生成器，性能最高
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, n - 1);

    size_t startIdx = dist(gen);

    for (size_t i = 0; i < n; ++i)
    {
        size_t idx = (startIdx + i) % n;
        if (idx == selfIndex)
            continue; // 不偷自己

        // 如果别人正在处理，偷取者不应该死等，而是换一家偷
        if (shards[idx].TryPop(actor))
        {
            return true;
        }
    }
    return false;
}