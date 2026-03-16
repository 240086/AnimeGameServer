#pragma once

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

class DatabaseTask;

class SaveQueue
{
public:

    static SaveQueue& Instance();

    void Push(std::unique_ptr<DatabaseTask> task);

    std::unique_ptr<DatabaseTask> Pop();

private:

    std::queue<std::unique_ptr<DatabaseTask>> queue_;

    std::mutex mutex_;

    std::condition_variable cond_;
};