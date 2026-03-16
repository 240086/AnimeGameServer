#include "database/worker/DBWorker.h"

#include "database/queue/SaveQueue.h"
#include "database/task/DatabaseTask.h"
#include "database/mysql/MySQLConnectionPool.h"

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
    if (!running_)
        return;
    running_ = false;

    // 【关键适配 1】：必须向自己负责的那个分片投送 nullptr
    SaveQueue::Instance().Push(shardIndex_, nullptr);

    if (thread_.joinable())
        thread_.join();
}

void DBWorker::Run()
{
    while (running_)
    {
        // 【关键适配 2】：只弹出自己负责分片的数据
        auto task = SaveQueue::Instance().Pop(shardIndex_);

        if (!task)
            break;

        auto conn = MySQLConnectionPool::Instance().Acquire();
        if (conn)
        {
            task->Execute(conn.get());
        }
    }
}