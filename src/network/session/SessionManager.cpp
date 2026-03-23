#include "network/session/SessionManager.h"
#include "network/Connection.h"
#include "game/player/PlayerManager.h"
#include "common/logger/Logger.h"
#include "common/metrics/ServerMetrics.h"
#include "network/session/Session.h"

SessionManager &SessionManager::Instance()
{
    static SessionManager instance;
    return instance;
}

// 2. 新增核心接口，供 LoginService (网关模式) 使用
std::shared_ptr<Session> SessionManager::CreateSessionWithId(uint64_t id)
{
    size_t idx = GetBucketIndex(id);
    auto &bucket = buckets_[idx];

    std::shared_ptr<Session> session;
    {
        std::lock_guard<std::mutex> lock(bucket.mutex);

        // 防御性编程：检查是否已存在（防止网关重发登录包导致重复创建）
        auto it = bucket.sessions.find(id);
        if (it != bucket.sessions.end())
        {
            return it->second;
        }

        session = std::make_shared<Session>(id);
        bucket.sessions[id] = session;
    }

    // 依然保留你原有的度量统计
    ServerMetrics::Instance().IncSession();
    LOG_DEBUG("Session created: sid={}", id);

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

void SessionManager::BindPlayerToSession(uint64_t playerId, std::shared_ptr<Session> session)
{
    std::lock_guard<std::mutex> lock(player_map_mutex_);
    player_to_session_[playerId] = session;
}

void SessionManager::UnbindPlayerFromSession(uint64_t playerId)
{
    std::lock_guard<std::mutex> lock(player_map_mutex_);
    player_to_session_.erase(playerId);
}

void SessionManager::KickPlayer(uint64_t playerId, uint64_t excludeSessionId, const std::string &reason)
{
    std::shared_ptr<Session> oldSession;
    {
        std::lock_guard<std::mutex> lock(player_map_mutex_);
        if (auto it = player_to_session_.find(playerId); it != player_to_session_.end())
        {
            oldSession = it->second.lock(); // 🔥 防悬空安全获取
        }
    }

    if (oldSession && oldSession->GetSessionId() != excludeSessionId)
    {
        LOG_WARN("Kicking OLD player {} (Session {}), current NEW Session is {}, reason: {}",
                 playerId, oldSession->GetSessionId(), excludeSessionId, reason);

        if (auto conn = oldSession->GetConnection())
        {
            conn->Close();
        }
    }
    else if (oldSession && oldSession->GetSessionId() == excludeSessionId)
    {
        // 这种情况通常发生在客户端重试过快，或者逻辑层映射更新超前
        LOG_INFO("Skip kicking for player {} because it's the SAME session {}", playerId, excludeSessionId);
    }
}

void SessionManager::RemoveSession(uint64_t id)
{
    std::shared_ptr<Session> session;
    // 1. 从容器移除 Session
    size_t idx = id % BUCKET_COUNT;
    {
        std::lock_guard<std::mutex> lock(buckets_[idx].mutex);
        if (auto it = buckets_[idx].sessions.find(id); it != buckets_[idx].sessions.end())
        {
            session = it->second;
            buckets_[idx].sessions.erase(it);
        }
    }

    if (session)
    {
        auto player = session->GetPlayer();
        if (player)
        {
            uint64_t uid = player->GetId();

            // 2. 🔥 自动解绑映射表，保证数据一致性
            UnbindPlayerFromSession(uid);

            // 3. 🔥 汇聚点：触发 Player 登出与最终存盘
            PlayerManager::Instance().Logout(uid);

            LOG_INFO("Session {} closed, Player {} logged out via Unified Path.", id, uid);
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
        LOG_WARN("Session {} timeout, closing connection.", session->GetSessionId());

        // 我们不在这里调用 Logout
        // 我们通过 conn->Close() 或直接调用 RemoveSession 的逻辑来保证单向流动
        auto conn = session->GetConnection();
        if (conn)
        {
            conn->Close();
        }

        // 如果连接已经失效，强制走一遍清理确保资源回收
        RemoveSession(session->GetSessionId());
    }
}