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
    // 这里的批次大小 32 是平衡延迟和吞吐量的经验值
    constexpr size_t kMaxBatchSize = 32;

    while (true)
    {
        // 1. 批量获取任务，减少队列锁竞争
        auto tasks = SaveQueue::Instance().PopBatch(shardIndex_, kMaxBatchSize);
        if (tasks.empty())
        {
            // 如果队列暂时为空，PopBatch 内部应该有 condition_variable 等待
            // 这里加上判断是为了防御性编程
            continue;
        }

        // 2. 为这一批任务获取一次数据库连接
        auto conn = MySQLConnectionPool::Instance().Acquire();
        if (!conn)
        {
            LOG_ERROR("DBWorker {} failed to acquire MySQL connection", shardIndex_);
            // 连接池耗尽时不能丢任务：将本批任务回灌队列，等待后续重试
            for (auto &task : tasks)
            {
                if (!task)
                    continue;
                SaveQueue::Instance().PushToShard(shardIndex_, std::move(task));
            }
            // 简单规避，防止因连接池耗尽导致的死循环 CPU 占用
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 3. 顺序执行批次内的任务
        for (auto &task : tasks)
        {
            // 4. 核心：检查是否收到退出指令
            if (!task)
            {
                LOG_INFO("DBWorker {} received stop signal, exiting...", shardIndex_);
                return; // 直接退出 Run 函数，结束线程
            }

            // 5. 执行具体的 SQL 逻辑并统计耗时
            auto start = std::chrono::high_resolution_clock::now();

            try
            {
                task->Execute(conn.get());
            }
            catch (const std::exception &e)
            {
                LOG_ERROR("DBWorker {} task execution error: {}", shardIndex_, e.what());
            }

            auto end = std::chrono::high_resolution_clock::now();

            uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

            // 6. 更新实时监控指标
            ServerMetrics::Instance().AddDBLatency(us);
            ServerMetrics::Instance().IncDBTaskFinished();
        }

        // 出了作用域后，conn (unique_ptr) 会自动归还给连接池
    }
}