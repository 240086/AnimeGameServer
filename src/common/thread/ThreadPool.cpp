#include "common/thread/ThreadPool.h"

ThreadPool::ThreadPool(size_t threadCount)
    : stop_(false)
{
    for (size_t i = 0; i < threadCount; ++i)
    {
        workers_.emplace_back([this]()
        {
            Worker();
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
    }

    cond_.notify_all();

    for (auto& t : workers_)
    {
        t.join();
    }
}

void ThreadPool::Enqueue(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
    }

    cond_.notify_one();
}

void ThreadPool::Worker()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mutex_);

            cond_.wait(lock, [this]()
            {
                return stop_ || !tasks_.empty();
            });

            if (stop_ && tasks_.empty())
                return;

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task();
    }
}