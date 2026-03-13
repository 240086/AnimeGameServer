#include "network/Connection.h"
#include "common/logger/Logger.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/manager/ConnectionManager.h"
#include "network/session/SessionManager.h"

Connection::Connection(boost::asio::io_context &ioContext)
    : socket_(ioContext)
{
    last_active_ = std::chrono::steady_clock::now();
}

Connection::tcp::socket &Connection::GetSocket()
{
    return socket_;
}

void Connection::Start()
{
    last_active_ = std::chrono::steady_clock::now();

    auto session = SessionManager::Instance().CreateSession();

    session->BindConnection(shared_from_this());

    SetSessionId(session->GetSessionId());

    LOG_INFO("client connected");
    DoRead();
}

void Connection::HandlePacket(uint16_t msgId, const char *data, size_t len)
{
    MessageDispatcher::Instance().Dispatch(msgId, this, data, len);
}

void Connection::DoRead()
{
    auto self(shared_from_this());
    last_active_ = std::chrono::steady_clock::now();

    socket_.async_read_some(
        boost::asio::buffer(buffer_, BUFFER_SIZE),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                recv_buffer_.Append(buffer_, length);

                parser_.Parse(
                    recv_buffer_,
                    [this](uint16_t msgId, const char *data, size_t len)
                    {
                        HandlePacket(msgId, data, len);
                    });

                DoRead();
            }
            else
            {
                LOG_WARN("client disconnected");
                socket_.close();
            }
        });
}