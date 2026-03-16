#include "database/worker/DBWorkerPool.h"
#include "database/queue/SaveQueue.h"

void DBWorkerPool::Start(size_t workerCount)
{
    if (isStarted_.exchange(true))
        return; // 防止重复启动

    // 专业建议：workerCount 最好等于 SaveQueue 的 SHARD_COUNT
    // 如果 workerCount < SHARD_COUNT，一个 Worker 就得管多个 Shard（逻辑会变复杂）
    // 如果 workerCount > SHARD_COUNT，会有 Worker 闲着
    size_t shardCount = SaveQueue::Instance().GetShardCount();
    size_t actualWorkers = std::min(workerCount, shardCount);

    for (size_t i = 0; i < actualWorkers; ++i)
    {
        // 创建时传入索引 i
        auto worker = std::make_unique<DBWorker>(i);
        worker->Start();
        workers_.push_back(std::move(worker));
    }
}

void DBWorkerPool::Stop()
{
    if (!isStarted_.exchange(false))
        return;

    for (auto &worker : workers_)
    {
        worker->Stop();
    }
    workers_.clear();
}