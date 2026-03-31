#pragma once

#include <atomic>
#include <functional>
#include <vector>
#include "game/actor/ObjectPool.h"

class Mailbox
{
public:
    using Task = std::function<void()>;
    static constexpr size_t MAX_MAILBOX = 2048;

    Mailbox()
    {
        Node *dummy = new Node();
        head_ = dummy;
        tail_.store(dummy, std::memory_order_relaxed);
    }

    ~Mailbox()
    {
        // 清理所有节点
        Node *node = head_;
        while (node)
        {
            Node *next = node->next;
            pool_.Release(node);
            node = next;
        }
    }

    // 🔴 MPSC Push
    bool PushAndCheckWasEmpty(Task task, bool &wasEmpty)
    {
        if (approx_size_.load(std::memory_order_relaxed) >= MAX_MAILBOX)
            return false;

        Node *node = pool_.Acquire();
        node->task = std::move(task);
        node->next = nullptr;

        Node *prev = tail_.exchange(node, std::memory_order_acq_rel);
        prev->next = node;

        wasEmpty = (approx_size_.fetch_add(1, std::memory_order_acq_rel) == 0);

        return true;
    }

    // 🔴 单消费者批量 Pop
    size_t PopBatch(std::vector<Task> &tasks, size_t maxCount)
    {
        size_t count = 0;

        Node *head = head_;
        Node *next = head->next;

        while (next && count < maxCount)
        {
            tasks.push_back(std::move(next->task));

            Node *old = head;
            head = next;
            next = next->next;

            // 🔴 回收节点（替代 delete）
            pool_.Release(old);

            ++count;
        }

        head_ = head;

        if (count > 0)
        {
            approx_size_.fetch_sub(count, std::memory_order_acq_rel);
        }

        return count;
    }

    size_t SizeApprox() const
    {
        return approx_size_.load(std::memory_order_relaxed);
    }

private:
    struct Node
    {
        Task task;
        Node *next = nullptr;

        Node() = default;
        Node(Task &&t) : task(std::move(t)) {}
    };

    // 单消费者读
    Node *head_;

    // 多生产者写
    std::atomic<Node *> tail_;

    std::atomic<size_t> approx_size_{0};
    LockFreeObjectPool<Node> pool_;
};