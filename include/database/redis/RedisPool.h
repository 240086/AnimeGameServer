#pragma once

#include "database/redis/RedisClient.h"
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

class RedisPool
{
public:
    static RedisPool &Instance();

    // 初始化连接池
    bool Init(const std::string &host, int port, size_t poolSize);

    // 获取/归还连接
    std::shared_ptr<RedisClient> Acquire();
    std::shared_ptr<RedisClient> TryAcquireFor(std::chrono::milliseconds timeout);
    void Release(std::shared_ptr<RedisClient> client);

private:
    RedisPool() = default;
    ~RedisPool() = default;
    RedisPool(const RedisPool &) = delete;
    RedisPool &operator=(const RedisPool &) = delete;

private:
    std::queue<std::shared_ptr<RedisClient>> pool_;
    std::mutex mutex_;
    std::condition_variable cv_;

    std::string host_;
    int port_;
    size_t poolSize_ = 0;
};

// RAII 封装：自动获取和释放 Redis 连接
class RedisGuard
{
public:
    RedisGuard()
    {
        client_ = RedisPool::Instance().Acquire();
    }
    ~RedisGuard()
    {
        if (client_)
        {
            RedisPool::Instance().Release(client_);
        }
    }
    // 像指针一样使用
    RedisClient *operator->() { return client_.get(); }
    bool IsValid() const { return client_ != nullptr; }

private:
    std::shared_ptr<RedisClient> client_;
};