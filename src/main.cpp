#include <iostream>
#include "common/logger/Logger.h"
#include "common/config/Config.h"
#include "network/TcpServer.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "services/ServiceManager.h"
#include <boost/asio.hpp>
#include "common/thread/GlobalThreadPool.h"
#include "game/gacha/GachaSystem.h"
#include "services/GachaService.h"

void TestGacha()
{
    std::map<int,int> count;

    for(int i=0;i<100000;i++)
    {
        auto item = GachaSystem::Instance().DrawOnce();

        count[item.rarity]++;
    }

    for(auto& [rarity,c]:count)
    {
        std::cout << "rarity " << rarity
                  << " -> " << c << std::endl;
    }
}

int main()
{
    Logger::Init();

    if (!Config::Instance().Load("config/server.yaml"))
    {
        LOG_ERROR("load config failed");
        return -1;
    }

    ServiceManager::Instance().InitServices();
    printf("DEBUG: After InitServices...\n"); fflush(stdout);

    int port = Config::Instance().GetServerPort();
    printf("DEBUG: Port retrieved: %d\n", port); fflush(stdout);

    boost::asio::io_context ioContext;

    // --- 新增：信号监听逻辑 ---
    boost::asio::signal_set signals(ioContext, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code &error, int signal_number)
                       {
                           LOG_INFO("Capture signal {}, server shutting down...", signal_number);

                           // 停止网络循环
                           ioContext.stop();

                           // 如果有线程池，也可以在这里关闭
                           // GlobalThreadPool::Instance().Shutdown();
                       });
    // ------------------------

    TcpServer server(ioContext, port);
    printf("DEBUG: After TcpServer ctor...\n"); fflush(stdout);

    server.StartAccept();
    printf("DEBUG: After StartAccept...\n"); fflush(stdout);

    LOG_INFO("server start at port {}", port);

    /*
        调试消息系统
    */

    uint32_t len = 5;
    uint16_t id = 1;

    char test[11];

    memcpy(test, &len, 4);
    memcpy(test + 4, &id, 2);
    memcpy(test + 6, "hello", 5);

    MessageDispatcher::Instance().Dispatch(id, nullptr, "hello", 5);

    GlobalThreadPool::Instance().GetPool().Enqueue([]()
                                                   { LOG_INFO("thread pool test task"); });

    // for (int i = 0; i < 10; i++)
    // {
    //     auto item = GachaSystem::Instance().DrawOnce();

    //     LOG_INFO("draw result: {} rarity={}", item.name, item.rarity);
    // }

    TestGacha();

    ioContext.run();

    return 0;
}