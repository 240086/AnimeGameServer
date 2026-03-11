#pragma once

#include "services/BaseService.h"
#include "network/Connection.h"

class LoginService : public BaseService
{
public:

    static LoginService& Instance();

    void Init() override;

    void HandleLogin(Connection* conn, const char* data, size_t len);
};