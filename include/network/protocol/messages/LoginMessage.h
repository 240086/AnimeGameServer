#pragma once

#include "network/protocol/IMessage.h"
#include <string>

class LoginMessage : public IMessage
{
public:
    uint16_t GetMsgId() const override { return 1001; } // 示例

    std::string username;
    std::string password;
};