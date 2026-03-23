#pragma once
#include <functional>
#include <unordered_map>
#include <mutex>
#include <shared_mutex> // C++17 读写锁
#include <memory>
#include "network/protocol/IMessage.h"

class MessageDecoder
{
public:
    using DecodeFunc = std::function<std::shared_ptr<anime::IMessage>(const char *, size_t)>;

    static MessageDecoder &Instance()
    {
        static MessageDecoder instance;
        return instance;
    }

    // 注册解码器：独占锁保护（写）
    void Register(uint16_t msgId, DecodeFunc func)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        decoders_[msgId] = std::move(func);
    }

    // 执行解码：共享锁保护（读），允许多个 IO 线程并发解码
    std::shared_ptr<anime::IMessage> Decode(uint16_t msgId, const char *data, size_t len)
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = decoders_.find(msgId);
        if (it == decoders_.end())
        {
            return nullptr;
        }
        return it->second(data, len);
    }

private:
    MessageDecoder() = default;
    std::unordered_map<uint16_t, DecodeFunc> decoders_;
    std::shared_mutex mutex_;
};