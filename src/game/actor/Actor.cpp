#include "game/actor/Actor.h"
#include "game/actor/ActorSystem.h"

void Actor::Post(Task task)
{
    mailbox_.Push(std::move(task));

    bool expected = false;

    if (is_scheduled_.compare_exchange_strong(expected, true))
    {
        ActorSystem::Instance().Schedule(this);
    }
}

void Actor::Process(int maxTasks)
{
    Task task;

    int count = 0;

    while(count < maxTasks && mailbox_.Pop(task))
    {
        task();
        count++;
    }

    SetScheduled(false);
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