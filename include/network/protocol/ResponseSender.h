#pragma once

#include "network/protocol/MessageContext.h"
#include "network/protocol/InternalPacket.h"
#include "network/protocol/ClientPacket.h"
#include "common/logger/Logger.h"
#include "network/Connection.h"
#include <google/protobuf/message.h>
#include <string>
#include <vector>
#include <memory>

class ResponseSender
{
public:
    /**
     * @brief 场景 A：发送 Protobuf 对象
     * 自动完成序列化并根据上下文选择协议头
     */
    template <typename ProtoMsg>
    static void Send(const MessageContext &ctx, uint16_t msgId, const ProtoMsg &resp)
    {
        std::string payload;
        if (!resp.SerializeToString(&payload))
        {
            LOG_ERROR("PB Serialize failed: msgId={}", msgId);
            return;
        }
        SendPayload(ctx, msgId, std::move(payload));
    }

    /**
     * @brief 场景 B：发送原始二进制 Payload (解决幂等性/缓存回放)
     * 使用 std::string&& 配合移动语义减少内存拷贝
     */
    static void SendPayload(const MessageContext &ctx, uint16_t msgId, std::string &&payload)
    {
        if (!ctx.conn)
        {
            LOG_ERROR("Send failed: connection is null, msgId={}", msgId);
            return;
        }

        std::shared_ptr<std::vector<char>> data;

        if (ctx.sid > 1000000)
        { // 假设你的 sid 应该是从网关分配的小整数
            LOG_ERROR("SID ANOMALY DETECTED! sid={}, msgId={}", ctx.sid, msgId);
        }

        // 根据上下文的协议类型进行封包
        if (ctx.protoType == ProtocolType::INTERNAL)
        {
            data = BuildInternal(ctx, msgId, std::move(payload));
        }
        else
        {
            data = BuildClient(msgId, std::move(payload));
        }

        // 执行异步发送
        ctx.conn->SendRaw(data);
    }

    // 为了兼容性，保留 const string& 版本（可选）
    static void SendPayload(const MessageContext &ctx, uint16_t msgId, const std::string &payload)
    {
        std::string temp = payload;
        SendPayload(ctx, msgId, std::move(temp));
    }

private:
    /**
     * @brief 构建内部网关协议包 (16字节头 + Body)
     */
    static std::shared_ptr<std::vector<char>>
    BuildInternal(const MessageContext &ctx, uint16_t msgId, std::string &&payload)
    {
        InternalPacket pkt(ctx.sid, msgId, ctx.seqId);
        pkt.Append(std::move(payload)); // 💡 假设你的 Append 支持 move

        // 优化：直接在 Serialize 中通过移动构造 shared_ptr
        auto result = std::make_shared<std::vector<char>>(pkt.Serialize());
        return result;
    }

    /**
     * @brief 构建客户端原生协议包 (6字节头 + Body)
     */
    static std::shared_ptr<std::vector<char>>
    BuildClient(uint16_t msgId, std::string &&payload)
    {
        ClientPacket pkt;
        pkt.SetMessageId(msgId);
        pkt.Append(std::move(payload));

        auto result = std::make_shared<std::vector<char>>(pkt.Serialize());
        return result;
    }
};