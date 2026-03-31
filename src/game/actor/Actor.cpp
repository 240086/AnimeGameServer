#include "game/actor/Actor.h"
#include "game/actor/ActorSystem.h"

void Actor::Post(Task task)
{
    if (IsStopped())
        return;

    bool wasEmpty = false;

    // 🔴 不允许 silent drop（简单 backpressure）
    while (!mailbox_.PushAndCheckWasEmpty(std::move(task), wasEmpty))
    {
        std::this_thread::yield();
    }

    // 🔴 只在 empty -> non-empty 时触发调度
    if (wasEmpty)
    {
        if (TrySchedule())
        {
            ActorSystem::Instance().Schedule(shared_from_this());
        }
    }
}

void Actor::Process(int maxTasks)
{
    std::vector<Task> tasks;
    tasks.reserve(static_cast<size_t>(maxTasks));

    const size_t count = mailbox_.PopBatch(tasks, static_cast<size_t>(maxTasks));

    for (size_t i = 0; i < count; ++i)
    {
        auto &task = tasks[i];
        if (task)
            task();
    }

    // 🔴 释放调度权
    SetScheduled(false);

    // 🔴 双检（配合 wasEmpty 机制已经安全）
    if (HasMoreTasks())
    {
        if (TrySchedule())
        {
            ActorSystem::Instance().Schedule(shared_from_this());
        }
    }
}

bool Actor::HasMoreTasks()
{
    return mailbox_.SizeApprox() > 0;
}

size_t Actor::GetMailboxSize() const
{
    return mailbox_.SizeApprox();
}

bool Actor::TrySchedule()
{
    bool expected = false;
    return is_scheduled_.compare_exchange_strong(
        expected, true,
        std::memory_order_acq_rel);
}

void Actor::SetScheduled(bool v)
{
    is_scheduled_.store(v, std::memory_order_release);
}