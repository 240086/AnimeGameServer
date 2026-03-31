#pragma once

#include <cstdint>
#include <string>
#include <optional>

// 幂等状态机
enum class IdempotencyState
{
    FIRST_TIME,  // 第一次收到该请求，允许向下执行业务逻辑
    IN_PROGRESS, // 该请求正在被另一条线程/进程处理，应拦截并报错
    COMPLETED    // 该请求已处理完毕，可以直接下发缓存的回包
};

struct IdempotencyResult
{
    IdempotencyState state;
    std::optional<std::string> payload; // 仅在 COMPLETED 状态下，携带上次序列化好的回包
};

class IdempotencyService
{
public:
    static IdempotencyService &Instance();

    IdempotencyService(const IdempotencyService &) = delete;
    IdempotencyService &operator=(const IdempotencyService &) = delete;

    /**
     * @brief 核心接口：检查并锁定请求
     * @param ttlSeconds 锁的超时时间。通常设置较短（如 10 秒），防止进程 Crash 导致死锁
     */
    IdempotencyResult CheckAndLock(uint64_t playerId, const std::string &traceId, int ttlSeconds = 10);

    /**
     * @brief 业务完成：将真实的序列化回包存入 Redis，覆盖“处理中”的状态
     * @param ttlSeconds 结果缓存时间。默认 60 秒，兼顾重试回放与 Redis 写放大控制
     */
    bool SaveResult(uint64_t playerId, const std::string &traceId, const std::string &payload, int ttlSeconds = 60);

    /**
     * @brief 异常回滚：如果业务执行中途失败（例如前置条件不足，余额不够），必须释放锁
     * 允许客户端在补足条件后，使用相同的 traceId 重新发起请求
     */
    bool Unlock(uint64_t playerId, const std::string &traceId);

private:
    IdempotencyService() = default;
    ~IdempotencyService() = default;

    // 内部方法：对接 RedisKeyManager 生成统一规范的 Key
    std::string MakeKey(uint64_t playerId, const std::string &traceId) const;
};