#pragma once

#include <atomic>
#include <functional>
#include <vector>
#include <thread>
#include "game/actor/ObjectPool.h"

class Mailbox
{
public:
    using Task = std::function<void()>;
    static constexpr size_t MAX_MAILBOX = 2048;

    struct Node
    {
        Task task;
        // 🔴 必须是原子指针
        std::atomic<Node *> next{nullptr};

        Node() = default;
        Node(Task &&t) : task(std::move(t)) {}
    };

    Mailbox()
    {
        Node *dummy = new Node();
        head_ = dummy;
        tail_.store(dummy, std::memory_order_relaxed);
    }

    ~Mailbox()
    {
        Node *node = head_;
        while (node)
        {
            Node *next = node->next.load(std::memory_order_relaxed);
            GetPool().Release(node);
            node = next;
        }
    }

    // 🔴 MPSC Push
    bool PushAndCheckWasEmpty(Task task, bool &wasEmpty)
    {
        // 1. 容量检查
        if (approx_size_.load(std::memory_order_relaxed) >= MAX_MAILBOX)
            return false;

        Node *node = GetPool().Acquire();
        node->task = std::move(task);
        node->next.store(nullptr, std::memory_order_relaxed);

        // 🔴 必须在链接入队之前加数量，防止被并发 Pop 减成负数
        wasEmpty = (approx_size_.fetch_add(1, std::memory_order_acq_rel) == 0);

        // 🔴 核心入队逻辑 (Dmitry Vyukov 算法)
        Node *prev = tail_.exchange(node, std::memory_order_acq_rel);

        // 发布节点给消费者
        prev->next.store(node, std::memory_order_release);

        return true;
    }

    // 🔴 单消费者批量 Pop
    size_t PopBatch(std::vector<Task> &tasks, size_t maxCount)
    {
        size_t count = 0;
        Node *head = head_;
        Node *next = head->next.load(std::memory_order_acquire);

        while (count < maxCount)
        {
            if (!next)
            {
                // 🔴 防假死漏洞修复：判断是否真的空了
                // 如果 head 不等于 tail，说明有个生产者正在执行 exchange 和 next.store 之间
                if (head != tail_.load(std::memory_order_acquire))
                {
                    // 生产者被系统挂起了，消费者必须自旋等待，决不能当天空队列退出！
                    while ((next = head->next.load(std::memory_order_acquire)) == nullptr)
                    {
                        std::this_thread::yield();
                    }
                }
                else
                {
                    break; // 确实空了
                }
            }

            // 提取任务
            tasks.push_back(std::move(next->task));

            // 前进
            Node *old = head;
            head = next;
            next = head->next.load(std::memory_order_acquire);

            // 归还 Dummy Node（注意：此时 head 成了新的 Dummy Node）
            GetPool().Release(old);
            ++count;
        }

        head_ = head; // 消费者本地指针更新

        if (count > 0)
        {
            approx_size_.fetch_sub(count, std::memory_order_release);
        }

        return count;
    }

    size_t SizeApprox() const
    {
        return approx_size_.load(std::memory_order_relaxed);
    }

private:
    // 🔴 修复对象池作用域：所有 Mailbox 必须共享一个静态池
    static LockFreeObjectPool<Node> &GetPool()
    {
        static LockFreeObjectPool<Node> global_pool;
        return global_pool;
    }

    // 🔴 避免伪共享 (False Sharing)：强迫读写变量分不到不同的 CPU 缓存行 (Cache Line)
    alignas(64) Node *head_;                         // 仅消费者频繁读写
    alignas(64) std::atomic<Node *> tail_;           // 生产者频繁写，消费者偶尔读
    alignas(64) std::atomic<size_t> approx_size_{0}; // 双方都会频繁读写
};