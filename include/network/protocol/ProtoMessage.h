// include/network/protocol/ProtoMessage.h
#pragma once
#include "network/protocol/IMessage.h"
#include <memory>

template <typename T>
class ProtoMessage : public IMessage
{
public:
    ProtoMessage(uint16_t msgId, std::shared_ptr<T> pb)
        : msgId_(msgId), pb_(std::move(pb)) {}

    uint16_t GetMsgId() const override { return msgId_; }

    const T &Get() const { return *pb_; }
    T &Mutable() { return *pb_; }
    std::shared_ptr<T> GetPtr() const { return pb_; }

private:
    uint16_t msgId_;
    std::shared_ptr<T> pb_;
};