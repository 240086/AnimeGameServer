#include "network/session/SessionManager.h"
#include "network/Connection.h"

SessionManager &SessionManager::Instance()
{
    static SessionManager instance;
    return instance;
}

std::shared_ptr<Session> SessionManager::CreateSession()
{
    uint64_t id = next_session_id_++;

    auto session = std::make_shared<Session>(id);

    size_t idx = GetBucketIndex(id);

    {
        std::lock_guard<std::mutex> lock(buckets_[idx].mutex);
        buckets_[idx].sessions[id] = session;
    }

    return session;
}

std::shared_ptr<Session> SessionManager::GetSession(uint64_t id)
{
    size_t idx = GetBucketIndex(id);

    std::lock_guard<std::mutex> lock(buckets_[idx].mutex);

    auto &map = buckets_[idx].sessions;

    auto it = map.find(id);

    if (it != map.end())
        return it->second;

    return nullptr;
}

void SessionManager::RemoveSession(uint64_t id)
{
    size_t idx = GetBucketIndex(id);

    std::shared_ptr<Session> session;

    {
        std::lock_guard<std::mutex> lock(buckets_[idx].mutex);

        auto &map = buckets_[idx].sessions;

        auto it = map.find(id);

        if (it == map.end())
            return;

        session = it->second;

        map.erase(it);
    }

    if (session)
    {
        session->UnbindActor();
        session->UnbindPlayer();
    }
}

void SessionManager::CheckTimeout()
{
    auto now = std::chrono::steady_clock::now();

    std::vector<std::shared_ptr<Session>> to_remove;

    for (size_t i = 0; i < BUCKET_COUNT; ++i)
    {
        std::lock_guard<std::mutex> lock(buckets_[i].mutex);

        auto &session_map = buckets_[i].sessions;

        for (auto it = session_map.begin(); it != session_map.end();)
        {
            auto session = it->second;

            auto diff = std::chrono::duration_cast<std::chrono::seconds>(
                            now - session->GetLastHeartbeat())
                            .count();

            if (diff > 60)
            {
                to_remove.push_back(session);
                ++it;
            }
        }
    }

    // 锁外关闭连接
    for (auto &session : to_remove)
    {
        auto conn = session->GetConnection();

        if (conn)
        {
            conn->Close();
        }
    }
}