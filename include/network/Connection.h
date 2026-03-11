#pragma once

#include <boost/asio.hpp>
#include <memory>

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using tcp = boost::asio::ip::tcp;

    explicit Connection(boost::asio::io_context& ioContext);

    tcp::socket& GetSocket();

    void Start();

private:
    void DoRead();

private:
    tcp::socket socket_;
    enum { BUFFER_SIZE = 1024 };
    char buffer_[BUFFER_SIZE];
};