#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <string>

#include "database/mysql/MySQLConnection.h"

class MySQLConnectionPool
{
public:
    static MySQLConnectionPool& Instance();

    bool Init(
        const std::string& host,
        int port,
        const std::string& user,
        const std::string& password,
        const std::string& db,
        size_t poolSize);

    // 🔥 增加默认超时时间，避免雪崩时线程死锁（例如设为 3000ms）
    std::shared_ptr<MySQLConnection> Acquire(int timeoutMs = 3000);

    // 删除拷贝构造和赋值操作，确保单例纯洁性
    MySQLConnectionPool(const MySQLConnectionPool&) = delete;
    MySQLConnectionPool& operator=(const MySQLConnectionPool&) = delete;

private:
    MySQLConnectionPool() = default;
    ~MySQLConnectionPool() = default;

    // 🔥 内部专用的回收接口，避免在 lambda 中处理复杂的锁逻辑和生命周期
    void Release(MySQLConnection* ptr);

private:
    std::queue<std::unique_ptr<MySQLConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> initialized_{false};
};