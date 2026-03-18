#pragma once

#include <boost/asio.hpp>
#include <memory>

#include "network/asio/AsioContextPool.h"

class TcpServer : public std::enable_shared_from_this<TcpServer>
{
public:
    using tcp = boost::asio::ip::tcp;

    TcpServer(
        boost::asio::io_context &mainContext,
        AsioContextPool &contextPool,
        int port);

    void StartAccept();

    void Stop();

private:
    void DoAccept();

    // void CheckHeartbeat();

private:
    boost::asio::io_context &mainContext_;

    AsioContextPool &contextPool_;

    tcp::acceptor acceptor_;

    boost::asio::steady_timer timer_;

    int acceptorCount_ = 4; // accept 并发数
    std::atomic<int> pendingAccept_{0};
    const int MAX_PENDING_ACCEPT = 1000;
};