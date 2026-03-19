#include "services/IdempotencyService.h"
#include "database/redis/RedisPool.h"
#include "database/redis/RedisKeyManager.h"
#include "common/logger/Logger.h"

IdempotencyService &IdempotencyService::Instance()
{
    static IdempotencyService instance;
    return instance;
}

// 内部 Key 生成逻辑：规范为 v1:lock:idempotency:{player}:{trace}
std::string IdempotencyService::MakeKey(uint64_t playerId, const std::string &traceId) const
{
    return "v1:lock:idempotency:{" + std::to_string(playerId) + "}:" + traceId;
}

IdempotencyResult IdempotencyService::CheckAndLock(uint64_t playerId, const std::string &traceId, int ttlSeconds)
{
    IdempotencyResult result;
    result.state = IdempotencyState::IN_PROGRESS; // 默认状态

    if (traceId.empty())
    {
        result.state = IdempotencyState::FIRST_TIME;
        return result;
    }

    auto redis = RedisPool::Instance().Acquire();
    if (!redis)
    {
        LOG_ERROR("Idempotency: Redis connection failed for player {}", playerId);
        result.state = IdempotencyState::FIRST_TIME; // Redis挂了，降级放行逻辑，避免玩家无法玩游戏
        return result;
    }

    std::string key = MakeKey(playerId, traceId);

    // 1. 尝试加锁 (SETNX)，初始标记为 "P" (Processing)
    // 这里的 SetNX 底层应实现为：SET key "P" EX ttlSeconds NX
    bool isNew = redis->SetNX(key, "P", ttlSeconds);

    if (isNew)
    {
        // 成功抢到锁，说明是第一次请求
        result.state = IdempotencyState::FIRST_TIME;
    }
    else
    {
        // 抢锁失败，说明 Key 已存在。执行 GET 检查状态
        auto val = redis->Get(key);
        if (!val.has_value() || *val == "P")
        {
            // 依然是 "P"，说明另一条线程还没跑完
            result.state = IdempotencyState::IN_PROGRESS;
        }
        else
        {
            // 拿到了具体的内容，说明是超时重试，直接回放结果
            result.state = IdempotencyState::COMPLETED;
            result.payload = *val;
        }
    }

    RedisPool::Instance().Release(redis);
    return result;
}

bool IdempotencyService::SaveResult(uint64_t playerId, const std::string &traceId, const std::string &payload, int ttlSeconds)
{
    if (traceId.empty())
        return true;

    auto redis = RedisPool::Instance().Acquire();
    if (!redis)
        return false;

    std::string key = MakeKey(playerId, traceId);

    // 将 PENDING 状态覆盖为真实的回包 Payload，并延长 TTL
    bool ok = redis->Set(key, payload, ttlSeconds);

    RedisPool::Instance().Release(redis);
    return ok;
}

bool IdempotencyService::Unlock(uint64_t playerId, const std::string &traceId)
{
    if (traceId.empty())
        return true;

    auto redis = RedisPool::Instance().Acquire();
    if (!redis)
        return false;

    std::string key = MakeKey(playerId, traceId);

    // 如果业务预校验失败（如钱不够），必须删掉这个锁，允许玩家下次带同样的 traceId 重试
    bool ok = redis->Del(key);

    RedisPool::Instance().Release(redis);
    return ok;
}