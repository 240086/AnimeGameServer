#include "game/actor/Actor.h"
#include "game/actor/ActorSystem.h"

void Actor::Post(Task task)
{
    if (IsStopped())
        return; // 拦截幽灵任务
    mailbox_.Push(std::move(task));

    // 使用 shared_from_this 保证在入队期间对象不会被销毁
    if (TrySchedule())
    {
        ActorSystem::Instance().Schedule(shared_from_this());
    }
}

void Actor::Process(int maxTasks)
{
    Task task;
    int count = 0;

    // 逻辑提纯：Process 只管做功，不负责调度状态的终结
    while (count < maxTasks && mailbox_.Pop(task))
    {
        if (task)
            task();
        count++;
    }

    // 2. 尝试重置调度位
    // 注意：这里不能直接 store(false)，必须配合 HasMoreTasks 检查
    SetScheduled(false);

    // 3. 核心修复：双重检查
    // 如果在 SetScheduled(false) 之后，mailbox 又有了新任务，
    // 必须尝试重新竞争调度权，否则该 Actor 就会“由于没人拉一把”而永久沉睡。
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