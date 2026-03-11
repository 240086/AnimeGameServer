#pragma once

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

private:
    void DoRead();

private:
    tcp::socket socket_;
    enum { BUFFER_SIZE = 1024 };
    char buffer_[BUFFER_SIZE];
    
    RecvBuffer recv_buffer_;
    PacketParser parser_;

    std::chrono::steady_clock::time_point last_active_;
};