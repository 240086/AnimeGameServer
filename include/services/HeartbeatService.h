#pragma once

#include "services/BaseService.h"
#include <cstddef>
#include <memory>
#include "network/protocol/IMessage.h"

class Connection;
class Player;
class HeartbeatService : public BaseService
{
public:
    static HeartbeatService &Instance();

    void Init();

private:
    void HandleHeartbeat(Connection *conn, Player *player, std::shared_ptr<IMessage> msg);
};