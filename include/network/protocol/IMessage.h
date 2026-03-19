#pragma once

#include <cstdint>

class IMessage
{
public:
    virtual ~IMessage() = default;

    virtual uint16_t GetMsgId() const = 0;
};