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

    session_id_ = session->GetSessionId();

    LOG_INFO("connection start session={}", session_id_);

    DoRead();
}

void Connection::HandlePacket(uint16_t msgId, const char *data, size_t len)
{
    MessageDispatcher::Instance().Dispatch(msgId, this, data, len);
}

void Connection::DoRead()
{
    auto self(shared_from_this());

    socket_.async_read_some(
        boost::asio::buffer(buffer_, sizeof(buffer_)),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                last_active_ = std::chrono::steady_clock::now();

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
                LOG_WARN("client disconnected conn={}", connection_id_);

                Close();
            }
        });
}

void Connection::SendPacket(const Packet &packet)
{
    auto data = std::make_shared<std::vector<char>>(packet.Serialize());

    auto self = shared_from_this();

    boost::asio::async_write(
        socket_,
        boost::asio::buffer(*data),
        [self, data](boost::system::error_code ec, std::size_t)
        {
            if (ec)
            {
                self->Close();
            }
        });
}

void Connection::Close()
{
    if (closed_.exchange(true))
        return;
    SessionManager::Instance().RemoveSession(session_id_);

    ConnectionManager::Instance().RemoveConnection(connection_id_);

    if (socket_.is_open())
    {
        boost::system::error_code ec;
        socket_.close(ec);
    }
}