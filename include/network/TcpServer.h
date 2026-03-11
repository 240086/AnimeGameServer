#pragma once

#include <boost/asio.hpp>

class TcpServer
{
public:
    using tcp = boost::asio::ip::tcp;

    TcpServer(boost::asio::io_context& ioContext, int port);

    void StartAccept();

private:
    void DoAccept();

private:
    boost::asio::io_context& ioContext_;
    tcp::acceptor acceptor_;
};