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

void ActorSystem::Schedule(std::shared_ptr<Actor> actor)
{
    if (!running_ || !actor)
        return;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        ready_queue_.push(std::move(actor));
    }
    cond_.notify_one();
}

void ActorSystem::Worker()
{
    while (true)
    {
        std::shared_ptr<Actor> actor = nullptr;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [this]
                       { return !running_ || !ready_queue_.empty(); });

            // 停机且队列空，才退出
            if (!running_ && ready_queue_.empty())
                return;

            if (!ready_queue_.empty())
            {
                actor = std::move(ready_queue_.front());
                ready_queue_.pop();
            }
        }

        if (actor)
        {
            // 核心逻辑：单次处理
            actor->Process();

            // 检查是否需要继续调度
            // 此时不需要在这里 SetScheduled(false)，
            // 因为 Actor 内部的 TrySchedule 应该是原子的
            if (actor->HasMoreTasks())
            {
                Schedule(actor);
            }
            else
            {
                // 只有在确定没任务时，才交出调度权
                actor->SetScheduled(false);

                // Double Check：防止在 SetScheduled(false) 的瞬间有新任务进入
                if (actor->HasMoreTasks() && actor->TrySchedule())
                {
                    Schedule(actor);
                }
            }
        }
    }
}