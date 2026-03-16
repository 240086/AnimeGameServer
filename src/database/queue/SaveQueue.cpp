#include "database/queue/SaveQueue.h"
#include "database/task/DatabaseTask.h"

SaveQueue &SaveQueue::Instance()
{
    static SaveQueue instance;
    return instance;
}

void SaveQueue::Push(std::unique_ptr<DatabaseTask> task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(task));
    }

    cond_.notify_one();
}

std::unique_ptr<DatabaseTask> SaveQueue::Pop()
{
    std::unique_lock<std::mutex> lock(mutex_);

    cond_.wait(lock, [this]
               { return !queue_.empty(); });

    auto task = std::move(queue_.front());
    queue_.pop();

    return task;
}