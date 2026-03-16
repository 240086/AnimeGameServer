#include "game/actor/Actor.h"
#include "game/actor/ActorSystem.h"

void Actor::Post(Task task)
{
    if (IsStopped())
        return;

    mailbox_.Push(std::move(task));

    if (TrySchedule())
    {
        ActorSystem::Instance().Schedule(shared_from_this());
    }
}

void Actor::Process(int maxTasks)
{
    Task task;
    int count = 0;

    // 1. 纯粹执行任务，最多执行 maxTasks 个，防止占用 Worker 太久（饥饿）
    while (count < maxTasks && mailbox_.Pop(task))
    {
        if (task)
            task();
        count++;
    }

    // 2. 释放调度权：这是最关键的一步
    SetScheduled(false);

    // 3. 唯一的 Double Check 屏障
    // 如果交出调度权后，mailbox 恰好进了新任务，尝试重新抢占调度权
    if (HasMoreTasks() && TrySchedule())
    {
        ActorSystem::Instance().Schedule(shared_from_this());
    }
}

bool Actor::HasMoreTasks()
{
    return mailbox_.Size() > 0;
}

bool Actor::TrySchedule()
{
    bool expected = false;
    return is_scheduled_.compare_exchange_strong(expected, true);
}

void Actor::SetScheduled(bool v)
{
    is_scheduled_.store(v);
}