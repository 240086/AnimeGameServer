#pragma once

#include <queue>
#include <mutex>
#include <functional>

class Mailbox
{
public:
    static constexpr size_t MAX_MAILBOX = 2048;
    using Task = std::function<void()>;

    bool Push(Task task)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.size() >= MAX_MAILBOX)
            return false;
        queue_.push(std::move(task));

        return true;
    }

    bool Pop(Task &task)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty())
            return false;

        task = std::move(queue_.front());
        queue_.pop();

        return true;
    }

    size_t Size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    std::queue<Task> queue_;

    std::mutex mutex_;
};