#include "database/redis/RedisPool.h"
#include "common/logger/Logger.h"

RedisPool &RedisPool::Instance()
{
    static RedisPool instance;
    return instance;
}

bool RedisPool::Init(const std::string &host, int port, size_t poolSize)
{
    std::lock_guard<std::mutex> lock(mutex_);
    host_ = host;
    port_ = port;
    poolSize_ = poolSize;

    for (size_t i = 0; i < poolSize_; ++i)
    {
        auto client = std::make_shared<RedisClient>();
        if (!client->Connect(host_, port_))
        {
            LOG_ERROR("RedisPool initial connect failed at client index {}", i);
            return false;
        }
        pool_.push(client);
    }

    LOG_INFO("RedisPool initialized. Host={}:{}, Size={}", host_, port_, poolSize_);
    return true;
}

std::shared_ptr<RedisClient> RedisPool::Acquire()
{
    std::unique_lock<std::mutex> lock(mutex_);
    // 等待直到池中有可用连接
    cv_.wait(lock, [this]()
             { return !pool_.empty(); });

    auto client = pool_.front();
    pool_.pop();
    return client;
}

std::shared_ptr<RedisClient> RedisPool::TryAcquireFor(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!cv_.wait_for(lock, timeout, [this]()
                      { return !pool_.empty(); }))
    {
        return nullptr;
    }

    auto client = pool_.front();
    pool_.pop();
    return client;
}

void RedisPool::Release(std::shared_ptr<RedisClient> client)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (client)
    {
        pool_.push(client);
        cv_.notify_one(); // 通知一个正在等待的线程
    }
}