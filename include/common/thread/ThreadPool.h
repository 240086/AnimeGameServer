#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool
{
public:

    explicit ThreadPool(size_t threadCount);

    ~ThreadPool();

    void Enqueue(std::function<void()> task);

private:

    void Worker();

private:

    std::vector<std::thread> workers_;

    std::queue<std::function<void()>> tasks_;

    std::mutex mutex_;

    std::condition_variable cond_;

    std::atomic<bool> stop_;
};