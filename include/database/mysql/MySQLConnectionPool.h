#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>

#include "database/mysql/MySQLConnection.h"

class MySQLConnectionPool
{
public:
    static MySQLConnectionPool &Instance();

    bool Init(
        const std::string &host,
        int port,
        const std::string &user,
        const std::string &password,
        const std::string &db,
        size_t poolSize);

    std::shared_ptr<MySQLConnection> Acquire();

private:
    std::queue<std::unique_ptr<MySQLConnection>> pool_;

    std::mutex mutex_;
    std::condition_variable cond_;

    std::atomic<bool> initialized_{false};
};