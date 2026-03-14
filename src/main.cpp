#include <iostream>
#include <thread>
#include <vector>
#include <csignal>

#include <boost/asio.hpp>

#include "common/logger/Logger.h"
#include "common/config/Config.h"
#include "network/TcpServer.h"
#include "services/ServiceManager.h"
#include "game/gacha/GachaPoolManager.h"
#include "game/actor/ActorSystem.h"
#include "network/asio/AsioContextPool.h"

int main()
{
    // 1. 初始化基础组件
    Logger::Init();

    if (!Config::Instance().Load("config/server.yaml"))
    {
        LOG_ERROR("load config failed");
        return -1;
    }

    // 2. 初始化业务系统
    ServiceManager::Instance().InitServices();
    GachaPoolManager::Instance().LoadConfig(
        Config::Instance().GetConfigDir() + "gacha_pool.yaml");

    int port = Config::Instance().GetServerPort();

    // 3. 网络基础设施
    boost::asio::io_context mainContext;
    
    // 信号集：监听退出信号
    boost::asio::signal_set signals(mainContext, SIGINT, SIGTERM);

    AsioContextPool contextPool(
        std::thread::hardware_concurrency()
    );

    // 4. 创建服务器（以共享指针管理，配合 shared_from_this）
    auto server = std::make_shared<TcpServer>(
        mainContext,
        contextPool,
        port
    );

    // 5. 注册信号处理逻辑（优雅退出的触发点）
    signals.async_wait(
        [&](const boost::system::error_code& ec, int signal_number)
        {
            if (!ec)
            {
                LOG_INFO("Shutdown signal ({}) received. Starting graceful exit...", signal_number);
                
                // A. 停止接受新连接
                // server 内部应提供 Stop 接口调用 acceptor_.close()
                server->Stop(); 

                // B. 停止业务逻辑处理
                ActorSystem::Instance().Stop();

                // C. 停止 IO 上下文
                contextPool.Stop();
                mainContext.stop();
            }
        });

    // 6. 启动服务
    server->StartAccept();
    LOG_INFO("AnimeGameServer started at port {}", port);

    // 启动线程池
    contextPool.Run();

    // 主线程阻塞在这里，处理信号和 Accept 事件
    mainContext.run();

    LOG_INFO("Server exited cleanly.");
    return 0;
}