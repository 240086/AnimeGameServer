#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>

#include "network/session/Session.h"

class SessionManager
{
public:

    static SessionManager& Instance();

    std::shared_ptr<Session> CreateSession();

    std::shared_ptr<Session> GetSession(uint64_t id);

    void RemoveSession(uint64_t id);

private:

    std::unordered_map<uint64_t, std::shared_ptr<Session>> sessions_;

    std::mutex mutex_;

    uint64_t next_session_id_ = 1;
};