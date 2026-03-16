#include "database/worker/DBWorker.h"

#include "database/queue/SaveQueue.h"
#include "database/task/DatabaseTask.h"
#include "database/mysql/MySQLConnectionPool.h"

DBWorker::DBWorker()
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
    running_ = false;
    SaveQueue::Instance().Push(nullptr);

    if (thread_.joinable())
        thread_.join();
}

void DBWorker::Run()
{
    while (running_)
    {
        auto task = SaveQueue::Instance().Pop();

        if (!task)
            break;

        auto conn = MySQLConnectionPool::Instance().Acquire();

        if (conn)
        {
            task->Execute(conn.get());
        }
    }
}