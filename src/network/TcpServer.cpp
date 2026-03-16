#include "network/TcpServer.h"

#include "network/Connection.h"
#include "network/manager/ConnectionManager.h"
#include "network/session/SessionManager.h"
#include "common/logger/Logger.h"

TcpServer::TcpServer(
    boost::asio::io_context &mainContext,
    AsioContextPool &contextPool,
    int port)
    : mainContext_(mainContext),
      contextPool_(contextPool),
      acceptor_(
          mainContext,
          tcp::endpoint(tcp::v4(), port)),
      timer_(mainContext, std::chrono::seconds(30))
{
}

void TcpServer::StartAccept()
{
    // CheckHeartbeat();
    DoAccept();
}

void TcpServer::DoAccept()
{
    auto self = shared_from_this();

    // 选择一个 IO 线程
    auto &ioContext = contextPool_.GetIOContext();

    auto connection =
        std::make_shared<Connection>(ioContext);

    acceptor_.async_accept(
        connection->GetSocket(),
        [self, connection](boost::system::error_code ec)
        {
            if (!ec)
            {
                int id =
                    ConnectionManager::Instance()
                        .AddConnection(connection);

                LOG_INFO(
                    "client connected id={} online={}",
                    id,
                    ConnectionManager::Instance().OnlineCount());

                connection->SetConnectionId(id);
                connection->Start();
            }
            else
            {
                LOG_ERROR("accept error {}", ec.message());
            }

            self->DoAccept();
        });
}

// void TcpServer::CheckHeartbeat()
// {
//     auto self = shared_from_this();

//     timer_.async_wait(
//         [self](const boost::system::error_code& ec)
//         {
//             if (ec)
//                 return;

//             SessionManager::Instance().CheckTimeout();

//             LOG_INFO("heartbeat check running");

//             self->timer_.expires_after(
//                 std::chrono::seconds(30));

//             self->CheckHeartbeat();
//         });
// }

void TcpServer::Stop()
{
    boost::system::error_code ec;
    acceptor_.close(ec); // 停止监听
    timer_.cancel();     // 停止心跳检测
    LOG_INFO("TcpServer listener closed.");
}