#pragma once

#include <hiredis/hiredis.h>
#include <string>
#include <memory>
#include <optional>
#include <mutex> // 新增：支持锁机制

class RedisClient
{
public:
    RedisClient();
    ~RedisClient();

    // 禁止拷贝 (RAII 资源保护)
    RedisClient(const RedisClient &) = delete;
    RedisClient &operator=(const RedisClient &) = delete;

    bool Connect(const std::string &host, int port);

    bool SetNX(const std::string &key, const std::string &value, int expireSeconds);

    // 业务接口
    std::optional<std::string> Get(const std::string &key);
    bool Set(const std::string &key, const std::string &value, int expireSeconds = 0);

private:
    bool CheckConnection();

private:
    redisContext *ctx_ = nullptr;
    std::string host_;
    int port_ = 0;
    std::mutex mutex_; // 新增：互斥锁，保障单连接下的线程安全
};