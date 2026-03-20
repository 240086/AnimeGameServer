#pragma once
#include <boost/asio.hpp>
#include <memory>
#include "network/buffer/RecvBuffer.h"
#include "network/protocol/PacketParser.h"
#include <chrono>
#include "protocol/Packet.h"
#include <deque>
#include <atomic>

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using tcp = boost::asio::ip::tcp;
    using strand_type = boost::asio::strand<boost::asio::io_context::executor_type>;

    explicit Connection(boost::asio::io_context &ioContext)
        : socket_(ioContext),
          strand_(boost::asio::make_strand(ioContext))
    {
    }

    tcp::socket &GetSocket();

    void Start();

    void SendPacket(const Packet &packet);

    void Close();

    void SetConnectionId(uint64_t id) { connection_id_ = id; }
    void SetSessionId(uint64_t id) { session_id_ = id; }

    uint64_t GetSessionId() const { return session_id_; }
    uint64_t GetConnectionId() const { return connection_id_; }

private:
    void DoRead();
    void DoWrite();
    void HandlePacket(const MessageContext &ctx, const char *data, size_t len);

    // 🔥 新增
    void AsyncCleanup(uint64_t sid);

private:
    tcp::socket socket_;
    strand_type strand_;

    enum
    {
        BUFFER_SIZE = 8192
    };

    char buffer_[BUFFER_SIZE];

    RecvBuffer recv_buffer_;
    PacketParser parser_;

    uint64_t session_id_ = 0;
    uint64_t connection_id_ = 0;

    std::chrono::steady_clock::time_point last_active_;

    std::atomic<bool> closed_{false};

    std::deque<std::shared_ptr<std::vector<char>>> write_queue_;
};