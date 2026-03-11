#include "network/manager/ConnectionManager.h"

ConnectionManager& ConnectionManager::Instance()
{
    static ConnectionManager instance;
    return instance;
}

int ConnectionManager::AddConnection(std::shared_ptr<Connection> conn)
{
    std::lock_guard<std::mutex> lock(mutex_);

    int id = next_id_++;

    connections_[id] = conn;

    return id;
}

void ConnectionManager::RemoveConnection(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    connections_.erase(id);
}

std::shared_ptr<Connection> ConnectionManager::GetConnection(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = connections_.find(id);

    if (it == connections_.end())
        return nullptr;

    return it->second;
}

size_t ConnectionManager::OnlineCount()
{
    std::lock_guard<std::mutex> lock(mutex_);

    return connections_.size();
}