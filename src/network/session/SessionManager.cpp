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
std::shared_ptr<Session> SessionManager::CreateSessionWithId(uint32_t id)
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

std::shared_ptr<Session> SessionManager::GetSession(uint32_t id)
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

void SessionManager::KickPlayer(uint64_t playerId, uint32_t excludeSessionId, const std::string &reason)
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
        LOG_DEBUG("Skip kicking for player {} because it's the SAME session {}", playerId, excludeSessionId);
    }
}

void SessionManager::RemoveSession(uint32_t id)
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

            LOG_DEBUG("Session {} closed, Player {} logged out via Unified Path.", id, uid);
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

    // 1. 筛选超时 Session (锁粒度控制)
    for (size_t i = 0; i < BUCKET_COUNT; ++i)
    {
        std::lock_guard<std::mutex> lock(buckets_[i].mutex);
        for (auto it = buckets_[i].sessions.begin(); it != buckets_[i].sessions.end();)
        {
            // 💡 建议：超时时间 180s 提取到配置中
            if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second->GetLastHeartbeat()).count() > 180)
            {
                to_remove.push_back(it->second);
                it = buckets_[i].sessions.erase(it); // 锁内只做容器移除
            }
            else
                ++it;
        }
    }

    // 2. 锁外执行逻辑清理 (关键修改)
    for (auto &session : to_remove)
    {
        uint32_t sid = session->GetSessionId();
        LOG_WARN("[SessionManager] Session {} timeout. Cleaning up logic context.", sid);

        // 💡 修复：千万不要调用 conn->Close() !!
        // 我们只需要通知业务层该玩家下线了

        // 如果有绑定的 Player 对象，执行存盘和资源卸载
        auto player = session->GetPlayer();
        if (player)
        {
            PlayerManager::Instance().AsyncSavePlayer(player);
            UnbindPlayerFromSession(player->GetId());
        }

        // 💡 进阶：如果需要告知网关该 Session 已失效，可以发一个特殊的协议包
        /*
        auto conn = session->GetConnection();
        if (conn) {
            auto kick_notify = std::make_shared<InternalPacket>(sid, MSG_ID_SESSION_KICK, 0);
            conn->Send(kick_notify);
        }
        */

        // 清理 Actor 绑定等
        session->UnbindActor();
        session->UnbindPlayer();

        // 💡 注意：由于上面已经从 buckets 删除了，不需要再调用 RemoveSession
    }
}