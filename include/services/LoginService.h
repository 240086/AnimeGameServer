#pragma once

#include "services/BaseService.h"
#include "network/Connection.h"
#include "network/protocol/IMessage.h"
#include "network/protocol/MessageContext.h"

class LoginService : public BaseService
{
public:
    static LoginService &Instance();

    void Init() override;

    void HandleLogin(const MessageContext &ctx,
                     std::shared_ptr<anime::IMessage> msg);
};