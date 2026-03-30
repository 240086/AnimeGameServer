#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <vector>

#include "network/session/Session.h"

class SessionManager
{
public:
    static SessionManager &Instance();

    std::shared_ptr<Session> CreateSessionWithId(uint32_t id);

    std::shared_ptr<Session> GetSession(uint32_t id);

    void RemoveSession(uint32_t id);

    void CheckTimeout();

    // 🔥 新增：通过 PlayerId 查找并踢掉旧 Session
    void KickPlayer(uint64_t playerId, uint32_t excludeSessionId, const std::string &reason);

    // 在 Session 绑定 Player 时调用，建立映射
    void BindPlayerToSession(uint64_t playerId, std::shared_ptr<Session> session);

    // 在 Session 销毁时调用，解除映射
    void UnbindPlayerFromSession(uint64_t playerId);

private:
    SessionManager() = default;

    static constexpr size_t BUCKET_COUNT = 64;

    struct Bucket
    {
        std::mutex mutex;
        std::unordered_map<uint32_t, std::shared_ptr<Session>> sessions;
    };

    Bucket buckets_[BUCKET_COUNT];

private:
    size_t GetBucketIndex(uint32_t id) const
    {
        return id % BUCKET_COUNT;
    }

    // 专门用于顶号查询的映射表（同样建议分桶或使用独立锁）
    std::mutex player_map_mutex_;
    std::unordered_map<uint64_t, std::weak_ptr<Session>> player_to_session_;
};