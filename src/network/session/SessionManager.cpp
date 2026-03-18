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

        auto &map = buckets_[idx].sessions;

        auto it = map.find(id);

        if (it == map.end())
            return;

        session = it->second;

        map.erase(it);
    }

    if (session)
    { // 1. 先通过 Session 拿到关联的 Player
        auto player = session->GetPlayer();
        if (player)
        {
            auto removedPlayer = PlayerManager::Instance().RemovePlayer(player->GetId());

            if (removedPlayer)
            {
                PlayerManager::Instance().AsyncSavePlayer(removedPlayer);
            }

            LOG_INFO("Player {} session {} closed. Async save scheduled.", player->GetId(), id);
        }

        // 3. 执行解绑，彻底清理 Session 内部持有的 Actor 引用
        session->UnbindActor();
        session->UnbindPlayer();
    }
    ServerMetrics::Instance().DecSession();
}

void SessionManager::CheckTimeout()
{
    auto now = std::chrono::steady_clock::now();
    std::vector<std::shared_ptr<Session>> to_remove;

    for (size_t i = 0; i < BUCKET_COUNT; ++i)
    {
        // 1. 锁内只做筛选，快速拷贝出超时的 Session 指针
        {
            std::lock_guard<std::mutex> lock(buckets_[i].mutex);
            auto &session_map = buckets_[i].sessions;

            for (auto it = session_map.begin(); it != session_map.end();)
            {
                auto session = it->second;
                auto diff = std::chrono::duration_cast<std::chrono::seconds>(
                                now - session->GetLastHeartbeat())
                                .count();

                if (diff > 180)
                {
                    to_remove.push_back(session);
                    // 2. 注意：这里直接 erase 还是在 RemoveSession 里 erase？
                    // 建议：此处直接标记，由下方的 Close 触发事件回调
                    it = session_map.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    // 3. 锁外执行耗时或可能引起回调的操作
    for (auto &session : to_remove)
    {
        LOG_WARN("Session {} timeout (180s), kicking player.", session->GetSessionId());

        // 触发最终存盘（通过回调或显式调用）
        auto player = session->GetPlayer();
        if (player)
        {
            auto removedPlayer = PlayerManager::Instance().RemovePlayer(player->GetId());

            if (removedPlayer)
            {
                PlayerManager::Instance().AsyncSavePlayer(removedPlayer);
            }

            LOG_INFO("Player {} session {} closed. Async save scheduled.", player->GetId(), session->GetSessionId());
        }

        auto conn = session->GetConnection();
        if (conn)
            conn->Close();

        session->UnbindActor();
        session->UnbindPlayer();
    }
}