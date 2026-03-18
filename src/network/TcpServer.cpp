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
      acceptor_(mainContext),
      timer_(mainContext, std::chrono::seconds(30))
{
    tcp::endpoint endpoint(tcp::v4(), port);

    boost::system::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec)
        throw std::runtime_error("acceptor open failed");

    acceptor_.set_option(boost::asio::socket_base::reuse_address(true));

    acceptor_.bind(endpoint, ec);
    if (ec)
        throw std::runtime_error("acceptor bind failed");

    // 🔥 核心：设置 backlog
    acceptor_.listen(8192, ec);
    if (ec)
        throw std::runtime_error("acceptor listen failed");
}

void TcpServer::StartAccept()
{
    for (int i = 0; i < acceptorCount_; ++i)
    {
        DoAccept();
    }
}

void TcpServer::DoAccept()
{
    auto self = shared_from_this();

    // 1. 获取一个 Worker 线程的 io_context
    auto &ioContext = contextPool_.GetIOContext();

    // 2. 创建连接，此时 connection 已经属于某个特定的 Worker 线程
    auto connection = std::make_shared<Connection>(ioContext);

    // 3. 开始异步接受
    acceptor_.async_accept(
        connection->GetSocket(),
        [self, connection](boost::system::error_code ec)
        {
            if (!ec)
            {
                // 🔥 【关键修改】：获取该连接原本所属的执行器（Worker 线程）
                auto executor = connection->GetSocket().get_executor();

                // 🔥 将“重操作”投递给对应的 Worker 线程，而不是 mainContext_！
                boost::asio::post(executor, [connection]()
                                  {
                    // 在 Worker 线程执行 Map 插入（配合我们之前的分桶锁，性能爆炸）
                    int id = ConnectionManager::Instance().AddConnection(connection);

                    connection->SetConnectionId(id);
                    
                    // 在 Worker 线程执行 async_read_some，正式开启监听
                    connection->Start();

                    // 日志采样，减少对 IO 线程的干扰
                    static std::atomic<int> counter{0};
                    if ((counter.fetch_add(1) % 100) == 0)
                    {
                        LOG_INFO("client connected id={} online={}",
                                 id, ConnectionManager::Instance().OnlineCount());
                    } });
            }
            else
            {
                LOG_ERROR("accept error {}", ec.message());
            }

            // 4. 立即继续 Accept，主线程不被任何业务逻辑干扰
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