#pragma once

#include <queue>
#include <mutex>
#include <functional>

class PlayerCommandQueue
{
public:

    using Command = std::function<void()>;

    void Push(Command cmd);

    void Execute();

private:

    std::queue<Command> queue_;

    std::mutex mutex_;
};