#pragma once

#include "services/BaseService.h"
#include <cstddef>
#include <memory>
#include "network/protocol/IMessage.h"
#include "network/protocol/MessageContext.h"

class Connection;
class Player;
class HeartbeatService : public BaseService
{
public:
    static HeartbeatService &Instance();

    void Init();

private:
    void HandleHeartbeat(const MessageContext &ctx, std::shared_ptr<anime::IMessage> msg);
};