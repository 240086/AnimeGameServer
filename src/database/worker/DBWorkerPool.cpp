#include "database/worker/DBWorkerPool.h"

void DBWorkerPool::Start(size_t workerCount)
{
    if (isStarted_.exchange(true))
        return; // 防止重复启动

    for (size_t i = 0; i < workerCount; ++i)
    {
        auto worker = std::make_unique<DBWorker>();
        // 建议：在这里可以给每个 worker 编号，以便后续做负载均衡
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