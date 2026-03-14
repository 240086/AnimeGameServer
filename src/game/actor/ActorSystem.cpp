#include "game/actor/ActorSystem.h"
#include "game/actor/Actor.h"

ActorSystem &ActorSystem::Instance()
{
    static ActorSystem instance;
    return instance;
}

void ActorSystem::Start(int threads)
{
    running_ = true;

    for (int i = 0; i < threads; i++)
    {
        workers_.emplace_back(&ActorSystem::Worker, this);
    }
}

void ActorSystem::Stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_)
            return;
        running_ = false; // 1. 先标记停止，后续 Post 将不再能通过 Schedule 唤醒（逻辑需配套）
    }

    cond_.notify_all();

    for (auto &t : workers_)
    {
        t.join();
    }

    workers_.clear();
}

void ActorSystem::Schedule(Actor *actor)
{
    if (!running_)
        return;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ready_queue_.push(actor);
    }

    cond_.notify_one();
}

void ActorSystem::Worker()
{
    while (running_)
    {
        Actor *actor = nullptr;

        {
            std::unique_lock<std::mutex> lock(mutex_);

            cond_.wait(lock, [this]
                       { return !running_ || !ready_queue_.empty(); });

            // 【核心修改】
            // 如果系统停止运行且队列已空，此时才真正退出线程
            if (!running_ && ready_queue_.empty())
            {
                return;
            }

            // 如果队列不为空，无论 running_ 是什么状态，都得继续拿任务
            if (!ready_queue_.empty())
            {
                actor = ready_queue_.front();
                ready_queue_.pop();
            }
        }

        if (actor)
        {
            actor->Process();

            if (actor->HasMoreTasks())
            {
                Schedule(actor);
            }
            else
            {
                actor->SetScheduled(false);

                if (actor->HasMoreTasks() && actor->TrySchedule())
                {
                    Schedule(actor);
                }
            }
        }
    }
}