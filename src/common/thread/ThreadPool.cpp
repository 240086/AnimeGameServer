#include "common/thread/ThreadPool.h"
#include "common/logger/Logger.h"

ThreadPool::ThreadPool(size_t threadCount)
    : stop_(false)
{
    for (size_t i = 0; i < threadCount; ++i)
    {
        workers_.emplace_back([this]()
                              { Worker(); });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
    }

    cond_.notify_all();

    for (auto &t : workers_)
    {
        t.join();
    }
}

void ThreadPool::Enqueue(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (stop_)
            return;

        tasks_.push(std::move(task));
    }
    task_count_++;
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
                       { return stop_ || !tasks_.empty(); });

            if (stop_ && tasks_.empty())
                return;

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        try
        {
            if (task)
            {
                task();
            }
        }
        catch (const std::exception &e)
        {
            // ✅ 专业做法：捕获后台任务异常，防止线程池意外崩溃
            LOG_ERROR("ThreadPool Task Exception: {}", e.what());
        }
        task_count_--; // 任务完成，计数减少
    }
}