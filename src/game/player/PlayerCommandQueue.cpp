#include "game/player/PlayerCommandQueue.h"

void PlayerCommandQueue::Push(Command cmd)
{
    std::lock_guard<std::mutex> lock(mutex_);

    queue_.push(std::move(cmd));
}

void PlayerCommandQueue::Execute()
{
    std::queue<Command> local;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        std::swap(local, queue_);
    }

    while (!local.empty())
    {
        auto& cmd = local.front();

        cmd();

        local.pop();
    }
}