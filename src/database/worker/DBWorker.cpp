#include "database/worker/DBWorker.h"

#include "database/queue/SaveQueue.h"
#include "database/task/DatabaseTask.h"
#include "database/mysql/MySQLConnectionPool.h"
#include "common/logger/Logger.h"

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
    if (!running_.compare_exchange_strong(expected, false)) {
        return;
    }

    // 2. 核心：精准投放退出信号
    // 我们不需要担心哈希路由问题，因为我们用了 PushToShard
    SaveQueue::Instance().PushToShard(shardIndex_, nullptr);

    // 3. 阻塞等待线程真正结束
    if (thread_.joinable()) {
        thread_.join();
    }
}
void DBWorker::Run()
{
    // 即使 running_ 已经为 false，只要队列里还有任务，我们就应该继续处理
    // 直到收到明确的“退出指令（nullptr）”
    while (true)
    {
        // 1. 阻塞等待任务。如果此时 Stop() 发送了 nullptr，wait 会被唤醒。
        auto task = SaveQueue::Instance().Pop(shardIndex_);

        // 2. 检查“毒丸”。这是唯一的退出出口。
        if (!task)
        {
            LOG_INFO("DBWorker {} received stop signal, exiting...", shardIndex_);
            break;
        }

        // 3. 正常的业务执行
        auto conn = MySQLConnectionPool::Instance().Acquire();
        if (conn)
        {
            task->Execute(conn.get());
        }
    }
}