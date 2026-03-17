#include "database/worker/DBWorker.h"

#include "database/queue/SaveQueue.h"
#include "database/task/DatabaseTask.h"
#include "database/mysql/MySQLConnectionPool.h"
#include "common/logger/Logger.h"
#include "common/metrics/ServerMetrics.h"
#include <chrono>

DBWorker::DBWorker(size_t shardIndex) : shardIndex_(shardIndex)
{
}

DBWorker::~DBWorker()
{
    Stop();
}

void DBWorker::Start()
{
    running_ = true;
    thread_ = std::thread(&DBWorker::Run, this);
}

void DBWorker::Stop()
{
    // 1. 使用原子操作防止重复停止
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false))
    {
        return;
    }

    // 2. 核心：精准投放退出信号
    // 我们不需要担心哈希路由问题，因为我们用了 PushToShard
    SaveQueue::Instance().PushToShard(shardIndex_, nullptr);

    // 3. 阻塞等待线程真正结束
    if (thread_.joinable())
    {
        thread_.join();
    }
}
void DBWorker::Run()
{
    constexpr size_t kMaxBatchSize = 32;

    while (true)
    {
        auto tasks = SaveQueue::Instance().PopBatch(shardIndex_, kMaxBatchSize);
        if (tasks.empty())
            continue;

        auto conn = MySQLConnectionPool::Instance().Acquire();
        if (!conn)
            continue;

        for (auto &task : tasks)
        {
            if (!task)
            {
                LOG_INFO("DBWorker {} received stop signal, exiting...", shardIndex_);
                return;
            }

            auto start = std::chrono::high_resolution_clock::now();
            task->Execute(conn.get());
            auto end = std::chrono::high_resolution_clock::now();

            uint64_t us =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    end - start)
                    .count();

            ServerMetrics::Instance().AddDBLatency(us);
            ServerMetrics::Instance().IncDBTaskFinished();
        }
    }
}
