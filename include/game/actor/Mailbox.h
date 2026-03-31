#pragma once

#include <queue>
#include <mutex>
#include <functional>
#include <atomic>
#include <vector>

class Mailbox
{
public:
    static constexpr size_t MAX_MAILBOX = 2048;
    using Task = std::function<void()>;

    // 🔴 新接口：返回 push 前是否为空
    bool PushAndCheckWasEmpty(Task task, bool &wasEmpty)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.size() >= MAX_MAILBOX)
        {
            return false;
        }

        wasEmpty = queue_.empty();
        queue_.push(std::move(task));
        approx_size_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    bool Pop(Task &task)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty())
            return false;

        task = std::move(queue_.front());
        queue_.pop();
        approx_size_.fetch_sub(1, std::memory_order_relaxed);
        return true;
    }

    size_t PopBatch(std::vector<Task> &tasks, size_t maxCount)
    {
        if (maxCount == 0)
            return 0;

        std::lock_guard<std::mutex> lock(mutex_);

        size_t popped = 0;
        while (!queue_.empty() && popped < maxCount)
        {
            tasks.push_back(std::move(queue_.front()));
            queue_.pop();
            ++popped;
        }

        approx_size_.fetch_sub(popped, std::memory_order_relaxed);
        return popped;
    }

    // 🔴 无锁 size（近似值）
    size_t SizeApprox() const
    {
        return approx_size_.load(std::memory_order_relaxed);
    }

private:
    std::queue<Task> queue_;
    mutable std::mutex mutex_;
    std::atomic<size_t> approx_size_{0};
};