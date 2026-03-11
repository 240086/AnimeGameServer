#include "network/Connection.h"
#include "common/logger/Logger.h"

Connection::Connection(boost::asio::io_context& ioContext)
    : socket_(ioContext)
{
}

Connection::tcp::socket& Connection::GetSocket()
{
    return socket_;
}

void Connection::Start()
{
    LOG_INFO("client connected");
    DoRead();
}

void Connection::DoRead()
{
    auto self(shared_from_this());

    socket_.async_read_some(
        boost::asio::buffer(buffer_, BUFFER_SIZE),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                LOG_INFO("received {} bytes", length);
                DoRead();
            }
            else
            {
                LOG_WARN("client disconnected");
            }
        });
}