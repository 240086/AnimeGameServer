#pragma once

#include "game/actor/Mailbox.h"
#include <atomic>
#include <memory>

class Actor : public std::enable_shared_from_this<Actor> // 1. 必须继承这个
{
public:
    using Task = Mailbox::Task;
    virtual ~Actor() = default;

    void Post(Task task);

    // Process 不再需要内部 SetScheduled，交给调度器或采用双重检查
    void Process(int maxTasks = 32);

    bool HasMoreTasks();
    bool TrySchedule();
    void SetScheduled(bool v);

    void Stop() { is_stopped_.store(true); }
    bool IsStopped() const { return is_stopped_.load(); }

private:
    Mailbox mailbox_;
    std::atomic<bool> is_scheduled_{false};
    std::atomic<bool> is_stopped_{false};
};