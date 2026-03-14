#pragma once

#include "game/actor/Mailbox.h"
#include <atomic>

class ActorSystem;

class Actor
{
public:

    using Task = Mailbox::Task;

    virtual ~Actor() = default;

    void Post(Task task);

    void Process(int maxTasks = 32);

    bool HasMoreTasks();

    bool TrySchedule();

    void SetScheduled(bool v);

private:

    Mailbox mailbox_;

    std::atomic<bool> is_scheduled_{false};
};