#pragma once
// F:\VSCode_project\Cpp_Proj\AnimeGameServer\include\network\Connection.h
#include <boost/asio.hpp>
#include <memory>
#include "network/buffer/RecvBuffer.h"
#include "network/protocol/PacketParser.h"
#include <chrono>

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using tcp = boost::asio::ip::tcp;

    explicit Connection(boost::asio::io_context& ioContext);

    tcp::socket& GetSocket();

    void Start();

    void HandlePacket(uint16_t msgId, const char* data, size_t len);

    void SetPlayerId(uint64_t id)
    {
        player_id_ = id;
    }

    uint64_t GetPlayerId() const
    {
        return player_id_;
    }

private:
    void DoRead();

private:
    tcp::socket socket_;
    enum { BUFFER_SIZE = 1024 };
    char buffer_[BUFFER_SIZE];
    
    RecvBuffer recv_buffer_;
    PacketParser parser_;

    uint64_t player_id_ = 0;

    std::chrono::steady_clock::time_point last_active_;
};