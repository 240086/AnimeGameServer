#include "database/mysql/MySQLConnectionPool.h"
#include "common/logger/Logger.h"

MySQLConnectionPool &MySQLConnectionPool::Instance()
{
    static MySQLConnectionPool instance;
    return instance;
}

bool MySQLConnectionPool::Init(
    const std::string &host,
    int port,
    const std::string &user,
    const std::string &password,
    const std::string &db,
    size_t poolSize)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // 防止重复初始化
    if (initialized_)
    {
        return true;
    }

    for (size_t i = 0; i < poolSize; ++i)
    {
        auto conn = std::make_unique<MySQLConnection>();

        if (!conn->Connect(host, port, user, password, db))
        {
            LOG_ERROR("Failed to initialize database pool connection.");
            return false;
        }

        pool_.push(std::move(conn));
    }

    initialized_ = true;
    return true;
}

std::shared_ptr<MySQLConnection> MySQLConnectionPool::Acquire(int timeoutMs)
{
    std::unique_ptr<MySQLConnection> conn;

    // 1. 缩小锁的作用域，仅在操作队列时加锁
    {
        std::unique_lock<std::mutex> lock(mutex_);

        // 带有超时机制的等待，防止数据库崩溃导致所有 Worker 线程卡死
        if (!cond_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]
                            { return !pool_.empty(); }))
        {
            LOG_ERROR("Acquire MySQL connection timeout ({} ms), queue might be exhausted", timeoutMs);
            return nullptr;
        }

        conn = std::move(pool_.front());
        pool_.pop();
    } // 🔓 解锁：后续的网络 I/O 操作将不会阻塞其他线程获取连接

    // 2. 检查连接健康状态 (网络 I/O 在锁外进行)
    if (!conn || mysql_ping(conn->Get()) != 0)
    {
        LOG_WARN("MySQL connection dead or invalid, attempting to reconnect...");

        if (!conn || !conn->Reconnect())
        {
            LOG_ERROR("MySQL reconnect failed during Acquire. Discarding this connection.");
            // 此时 conn 超出作用域会自动析构，连接总数会减少 1。
            // 生产环境中，此处可以考虑生成一个新的连接放入池中，或者依赖定期探活线程补充。
            return nullptr;
        }
        LOG_INFO("MySQL connection recovered successfully.");
    }

    // 3. 构建安全的 RAII 智能指针
    // 技巧：使用 Instance().Release() 避免任何指针捕获的生命周期风险
    return std::shared_ptr<MySQLConnection>(
        conn.release(),
        [](MySQLConnection *ptr)
        {
            MySQLConnectionPool::Instance().Release(ptr);
        });
}

void MySQLConnectionPool::Release(MySQLConnection *ptr)
{
    if (ptr == nullptr)
        return;

    std::unique_lock<std::mutex> lock(mutex_);
    pool_.push(std::unique_ptr<MySQLConnection>(ptr));
    cond_.notify_one();
}