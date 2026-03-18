#include "database/redis/RedisClient.h"
#include <iostream>
#include <cstring>

RedisClient::RedisClient() : ctx_(nullptr), port_(0) {}

RedisClient::~RedisClient()
{
    if (ctx_)
    {
        redisFree(ctx_);
        ctx_ = nullptr;
    }
}

bool RedisClient::Connect(const std::string &host, int port)
{
    host_ = host;
    port_ = port;

    if (ctx_)
    {
        redisFree(ctx_);
        ctx_ = nullptr;
    }

    struct timeval timeout = {1, 500000}; // 1.5s 连接超时
    ctx_ = redisConnectWithTimeout(host.c_str(), port, timeout);

    if (!ctx_ || ctx_->err)
    {
        std::cerr << "[Redis] Connect failed: " << (ctx_ ? ctx_->errstr : "null") << std::endl;
        return false;
    }

    redisSetTimeout(ctx_, timeout); // 设置读写超时
    return true;
}

bool RedisClient::SetNX(const std::string &key, const std::string &value, int expireSeconds)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!CheckConnection())
        return false;

    // 使用原子的 SET NX EX 指令
    redisReply *reply = (redisReply *)redisCommand(
        ctx_,
        "SET %s %b NX EX %d",
        key.c_str(),
        value.c_str(),
        (size_t)value.length(),
        expireSeconds);

    if (!reply)
        return false;

    // 如果设置成功，返回状态码回复 "OK"
    // 如果因 NX 条件不满足（已存在），返回 NIL 类型
    bool success = (reply->type == REDIS_REPLY_STATUS &&
                    strcasecmp(reply->str, "OK") == 0);

    freeReplyObject(reply);
    return success;
}

bool RedisClient::CheckConnection()
{
    // 逻辑优化：只有当 ctx 为空或 err 标记被置位时才重连
    if (ctx_ && ctx_->err == 0)
        return true;

    if (ctx_)
    {
        redisFree(ctx_);
        ctx_ = nullptr;
    }

    return Connect(host_, port_);
}

std::optional<std::string> RedisClient::Get(const std::string &key)
{
    std::lock_guard<std::mutex> lock(mutex_); // 止血方案：确保单 Context 的并发安全

    if (!CheckConnection())
        return std::nullopt;

    redisReply *reply = (redisReply *)redisCommand(ctx_, "GET %s", key.c_str());

    if (!reply)
        return std::nullopt;

    std::optional<std::string> result;
    if (reply->type == REDIS_REPLY_STRING)
    {
        result = std::string(reply->str, reply->len);
    }
    // REDIS_REPLY_NIL 会让 result 保持 nullopt

    freeReplyObject(reply);
    return result;
}

bool RedisClient::Set(const std::string &key, const std::string &value, int expireSeconds)
{
    std::lock_guard<std::mutex> lock(mutex_); // 止血方案

    if (!CheckConnection())
        return false;

    redisReply *reply = nullptr;

    if (expireSeconds > 0)
    {
        reply = (redisReply *)redisCommand(ctx_, "SETEX %s %d %b",
                                           key.c_str(),
                                           expireSeconds,
                                           value.c_str(),
                                           (size_t)value.length());
    }
    else
    {
        reply = (redisReply *)redisCommand(ctx_, "SET %s %b",
                                           key.c_str(),
                                           value.c_str(),
                                           (size_t)value.length());
    }

    if (!reply)
        return false;

    bool success = (reply->type == REDIS_REPLY_STATUS &&
                    (strcasecmp(reply->str, "OK") == 0));

    freeReplyObject(reply);
    return success;
}