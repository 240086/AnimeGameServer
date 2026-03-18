#pragma once

#include "game/actor/Mailbox.h"
#include <atomic>
#include <memory>

class Actor : public std::enable_shared_from_this<Actor>
{
public:
    using Task = Mailbox::Task;
    virtual ~Actor() = default;

    void Post(Task task);

    void Process(int maxTasks = 32);

    bool HasMoreTasks();
    bool TrySchedule();
    void SetScheduled(bool v);

    void Stop() { is_stopped_.store(true); }
    bool IsStopped() const { return is_stopped_.load(); }

    // 🔥 新增：路由Key（核心）
    virtual uint64_t GetRoutingKey() const = 0;

private:
    Mailbox mailbox_;
    std::atomic<bool> is_scheduled_{false};
    std::atomic<bool> is_stopped_{false};
};