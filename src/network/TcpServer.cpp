#include "network/TcpServer.h"
#include "network/Connection.h"
#include "common/logger/Logger.h"
#include "network/manager/ConnectionManager.h"
#include "network/session/SessionManager.h"

TcpServer::TcpServer(boost::asio::io_context &ioContext, int port)
    : ioContext_(ioContext),
      acceptor_(ioContext, tcp::endpoint(tcp::v4(), port)),
      timer_(ioContext, std::chrono::seconds(30))
{
    CheckHeartbeat();
}

void TcpServer::StartAccept()
{
    DoAccept();
}

void TcpServer::DoAccept()
{
    auto connection = std::make_shared<Connection>(ioContext_);

    acceptor_.async_accept(
        connection->GetSocket(),
        [this, connection](boost::system::error_code ec)
        {
            if (!ec)
            {
                int id = ConnectionManager::Instance().AddConnection(connection);

                LOG_INFO("client connected id={} online={}",
                         id,
                         ConnectionManager::Instance().OnlineCount());

                connection->Start();
            }

            DoAccept();
        });
}

void TcpServer::CheckHeartbeat()
{
    timer_.async_wait([this](const boost::system::error_code&)
    {
        SessionManager::Instance().CheckTimeout();

        LOG_INFO("heartbeat check running");

        timer_.expires_after(std::chrono::seconds(30));

        CheckHeartbeat();
    });
}