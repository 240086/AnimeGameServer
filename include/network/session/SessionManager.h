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

    static SessionManager& Instance();

    std::shared_ptr<Session> CreateSession();

    std::shared_ptr<Session> GetSession(uint64_t id);

    void RemoveSession(uint64_t id);

    void CheckTimeout();

private:

    SessionManager() = default;

    static constexpr size_t BUCKET_COUNT = 16;

    struct Bucket
    {
        std::mutex mutex;
        std::unordered_map<uint64_t, std::shared_ptr<Session>> sessions;
    };

    Bucket buckets_[BUCKET_COUNT];

    std::atomic<uint64_t> next_session_id_{1};

private:

    size_t GetBucketIndex(uint64_t id) const
    {
        return id % BUCKET_COUNT;
    }
};