#include "network/session/SessionManager.h"
#include "network/Connection.h"

SessionManager& SessionManager::Instance()
{
    static SessionManager instance;
    return instance;
}

std::shared_ptr<Session> SessionManager::CreateSession()
{
    std::lock_guard<std::mutex> lock(mutex_);

    uint64_t id = next_session_id_++;

    auto session = std::make_shared<Session>(id);

    sessions_[id] = session;

    return session;
}

std::shared_ptr<Session> SessionManager::GetSession(uint64_t id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(id);

    if (it != sessions_.end())
        return it->second;

    return nullptr;
}

void SessionManager::RemoveSession(uint64_t id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    sessions_.erase(id);
}

void SessionManager::CheckTimeout()
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();

    for (auto it = sessions_.begin(); it != sessions_.end(); )
    {
        auto session = it->second;

        auto diff = std::chrono::duration_cast<std::chrono::seconds>(
            now - session->GetLastHeartbeat()).count();

        if (diff > 60)
        {
            auto conn = session->GetConnection();

            if (conn)
            {
                conn->Close();
            }

            it = sessions_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}