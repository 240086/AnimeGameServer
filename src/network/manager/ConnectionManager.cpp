#include "network/manager/ConnectionManager.h"

ConnectionManager::ConnectionManager() : buckets_(BUCKET_COUNT) {}

ConnectionManager &ConnectionManager::Instance()
{
    static ConnectionManager instance;
    return instance;
}

int ConnectionManager::AddConnection(std::shared_ptr<Connection> conn)
{
    int id = next_id_.fetch_add(1, std::memory_order_relaxed);

    // 🔥 计算桶索引
    size_t idx = static_cast<size_t>(id) % BUCKET_COUNT;

    {
        std::lock_guard<std::mutex> lock(buckets_[idx].mutex);
        buckets_[idx].connections[id] = conn;
    }
    return id;
}

void ConnectionManager::RemoveConnection(int id)
{
    if (id <= 0)
        return;
    size_t idx = static_cast<size_t>(id) % BUCKET_COUNT;

    std::lock_guard<std::mutex> lock(buckets_[idx].mutex);
    buckets_[idx].connections.erase(id);
}

std::shared_ptr<Connection> ConnectionManager::GetConnection(int id)
{
    if (id <= 0)
        return nullptr;
    size_t idx = static_cast<size_t>(id) % BUCKET_COUNT;

    std::lock_guard<std::mutex> lock(buckets_[idx].mutex);
    auto it = buckets_[idx].connections.find(id);
    return (it != buckets_[idx].connections.end()) ? it->second : nullptr;
}

size_t ConnectionManager::OnlineCount()
{
    size_t total = 0;
    for (size_t i = 0; i < BUCKET_COUNT; ++i)
    {
        std::lock_guard<std::mutex> lock(buckets_[i].mutex);
        total += buckets_[i].connections.size();
    }
    return total;
}