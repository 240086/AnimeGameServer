#pragma once

#include "services/BaseService.h"
#include "network/Connection.h"

struct LoginResponse
{
    uint64_t playerId;
    uint64_t currency;
};

class LoginService : public BaseService
{
public:

    static LoginService& Instance();

    void Init() override;

    void HandleLogin(Connection* conn, const char* data, size_t len);
};