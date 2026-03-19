#pragma once

#include "services/BaseService.h"
#include "network/Connection.h"
#include "network/protocol/IMessage.h"

class LoginService : public BaseService
{
public:
    static LoginService &Instance();

    void Init() override;

    void HandleLogin(Connection *conn, std::shared_ptr<IMessage> msg);
};