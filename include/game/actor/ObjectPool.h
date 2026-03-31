#pragma once

#include <atomic>
#include <vector>

template <typename T>
class LockFreeObjectPool
{
public:
    LockFreeObjectPool() = default;

    ~LockFreeObjectPool()
    {
        T *node = global_head_.load();
        while (node)
        {
            T *next = node->next;
            delete node;
            node = next;
        }
    }

    // 🔴 获取对象
    T *Acquire()
    {
        // 1️⃣ 线程本地缓存
        if (!local_cache_.empty())
        {
            T *obj = local_cache_.back();
            local_cache_.pop_back();
            return obj;
        }

        // 2️⃣ 全局池（lock-free）
        T *head = global_head_.load(std::memory_order_acquire);

        while (head)
        {
            T *next = head->next;
            if (global_head_.compare_exchange_weak(
                    head, next,
                    std::memory_order_acq_rel))
            {
                return head;
            }
        }

        // 3️⃣ fallback
        return new T();
    }

    // 🔴 归还对象
    void Release(T *obj)
    {
        if (!obj)
            return;

        // 1️⃣ 放入线程本地缓存（最快路径）
        if (local_cache_.size() < kLocalCacheSize)
        {
            local_cache_.push_back(obj);
            return;
        }

        // 2️⃣ 放入全局池
        T *head = global_head_.load(std::memory_order_relaxed);

        do
        {
            obj->next = head;
        } while (!global_head_.compare_exchange_weak(
            head, obj,
            std::memory_order_release,
            std::memory_order_relaxed));
    }

private:
    static constexpr size_t kLocalCacheSize = 64;

    // 🔴 全局无锁链表
    std::atomic<T *> global_head_{nullptr};

    // 🔴 线程本地缓存（关键优化）
    static inline thread_local std::vector<T *> local_cache_{};
};
