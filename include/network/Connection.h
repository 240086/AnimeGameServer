#pragma once
// F:\VSCode_project\Cpp_Proj\AnimeGameServer\include\network\Connection.h
#include <boost/asio.hpp>
#include <memory>
#include "network/buffer/RecvBuffer.h"
#include "network/protocol/PacketParser.h"
#include <chrono>
#include "protocol/Packet.h"

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using tcp = boost::asio::ip::tcp;

    explicit Connection(boost::asio::io_context &ioContext);

    tcp::socket &GetSocket();

    void Start();

    void SetSessionId(uint64_t id)
    {
        session_id_ = id;
    }

    uint64_t GetSessionId() const
    {
        return session_id_;
    }

    void SendPacket(const Packet &packet);

    void SetConnectionId(uint64_t id)
    {
        connection_id_ = id;
    }

    void Close();

private:
    void DoRead();
    void HandlePacket(uint16_t msgId, const char *data, size_t len);

private:
    tcp::socket socket_;
    enum
    {
        BUFFER_SIZE = 4096
    };
    char buffer_[BUFFER_SIZE];

    RecvBuffer recv_buffer_;
    PacketParser parser_;

    uint64_t session_id_ = 0;
    uint64_t connection_id_ = 0;

    std::chrono::steady_clock::time_point last_active_;

    std::atomic<bool> closed_{false};
};