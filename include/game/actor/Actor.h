#pragma once

#include "game/actor/Mailbox.h"

class Actor
{
public:

    virtual ~Actor() = default;

    void Post(Mailbox::Task task)
    {
        mailbox_.Push(std::move(task));
    }

    void Process()
    {
        Mailbox::Task task;

        while(mailbox_.Pop(task))
        {
            task();
        }
    }

private:

    Mailbox mailbox_;
};