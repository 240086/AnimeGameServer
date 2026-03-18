#include "network/session/SessionManager.h"
#include "network/Connection.h"
#include "game/player/PlayerManager.h"
#include "common/logger/Logger.h"
#include "common/metrics/ServerMetrics.h"

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

    ServerMetrics::Instance().IncSession();

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
        auto it = buckets_[idx].sessions.find(id);
        if (it == buckets_[idx].sessions.end())
            return;

        session = it->second;
        buckets_[idx].sessions.erase(it);
    }

    if (session)
    {
        auto player = session->GetPlayer();
        if (player)
        {
            // 🔥 核心变更：调用 Logout 闭环
            // Logout 内部会执行：1. 从 PlayerManager 移除 2. 触发 AsyncSavePlayer(true)
            PlayerManager::Instance().Logout(player->GetId());
            LOG_INFO("Session {} removed, Player {} logged out and save scheduled.", id, player->GetId());
        }

        session->UnbindActor();
        session->UnbindPlayer();
    }
    ServerMetrics::Instance().DecSession();
}

void SessionManager::CheckTimeout()
{
    auto now = std::chrono::steady_clock::now();
    std::vector<std::shared_ptr<Session>> to_remove;

    // 1. 筛选超时 Session
    for (size_t i = 0; i < BUCKET_COUNT; ++i)
    {
        std::lock_guard<std::mutex> lock(buckets_[i].mutex);
        for (auto it = buckets_[i].sessions.begin(); it != buckets_[i].sessions.end();)
        {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second->GetLastHeartbeat()).count() > 180)
            {
                to_remove.push_back(it->second);
                it = buckets_[i].sessions.erase(it);
            }
            else
                ++it;
        }
    }

    // 2. 锁外执行踢人逻辑
    for (auto &session : to_remove)
    {
        auto player = session->GetPlayer();
        if (player)
        {
            LOG_WARN("Session {} timeout, kicking player {}.", session->GetSessionId(), player->GetId());
            // 🔥 统一走 Logout 接口
            PlayerManager::Instance().Logout(player->GetId());
        }

        auto conn = session->GetConnection();
        if (conn)
            conn->Close();

        session->UnbindActor();
        session->UnbindPlayer();
        ServerMetrics::Instance().DecSession();
    }
}