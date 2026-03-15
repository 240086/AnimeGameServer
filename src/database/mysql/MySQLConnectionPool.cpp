#include "database/mysql/MySQLConnectionPool.h"

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
    for (size_t i = 0; i < poolSize; ++i)
    {
        auto conn = std::make_unique<MySQLConnection>();

        if (!conn->Connect(host, port, user, password, db))
            return false;

        pool_.push(std::move(conn));
    }

    return true;
}

std::shared_ptr<MySQLConnection> MySQLConnectionPool::Acquire()
{
    std::unique_lock<std::mutex> lock(mutex_);

    cond_.wait(lock, [this]
               { return !pool_.empty(); });

    auto conn = std::move(pool_.front());
    pool_.pop();

    mysql_ping(conn->Get());

    return std::shared_ptr<MySQLConnection>(
        conn.release(),
        [this](MySQLConnection *ptr)
        {
            std::unique_lock<std::mutex> lock(mutex_);

            pool_.push(std::unique_ptr<MySQLConnection>(ptr));

            cond_.notify_one();
        });
}