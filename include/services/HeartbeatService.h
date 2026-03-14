#pragma once

#include "services/BaseService.h"
#include <cstddef>
class Connection;

class HeartbeatService : public BaseService
{
public:
    static HeartbeatService &Instance();

    void Init();

private:
    void HandleHeartbeat(
        Connection *conn,
        const char *data,
        size_t len);
};