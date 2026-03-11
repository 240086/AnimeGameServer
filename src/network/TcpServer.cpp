#include "network/TcpServer.h"
#include "network/Connection.h"
#include "common/logger/Logger.h"

TcpServer::TcpServer(boost::asio::io_context& ioContext, int port)
    : ioContext_(ioContext),
      acceptor_(ioContext, tcp::endpoint(tcp::v4(), port))
{
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
                connection->Start();
            }

            DoAccept();
        });
}