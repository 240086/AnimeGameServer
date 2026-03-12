#pragma once

#include <queue>
#include <mutex>
#include <functional>

class Mailbox
{
public:

    using Task = std::function<void()>;

    void Push(Task task)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(task));
    }

    bool Pop(Task& task)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if(queue_.empty())
            return false;

        task = std::move(queue_.front());
        queue_.pop();

        return true;
    }

private:

    std::queue<Task> queue_;

    std::mutex mutex_;
};