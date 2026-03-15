#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

class Connection;

class ConnectionManager
{
public:
    static ConnectionManager &Instance();

    int AddConnection(std::shared_ptr<Connection> conn);

    void RemoveConnection(int id);

    std::shared_ptr<Connection> GetConnection(int id);

    size_t OnlineCount();

private:
    ConnectionManager() = default;

private:
    std::unordered_map<int, std::shared_ptr<Connection>> connections_;

    std::mutex mutex_;

    std::atomic<int> next_id_{1};
};