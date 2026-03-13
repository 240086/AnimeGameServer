#include <iostream>
#include <boost/asio.hpp>

#include "common/logger/Logger.h"
#include "common/config/Config.h"
#include "network/TcpServer.h"
#include "services/ServiceManager.h"
#include "common/thread/GlobalThreadPool.h"
#include "game/gacha/GachaPoolManager.h"
#include "game/player/PlayerLogicLoop.h"
#include "game/player/PlayerManager.h"
#include "game/gacha/GachaSystem.h"

void TestGachaSimulation()
{
    std::cout << "=== Gacha Simulation Test ===" << std::endl;

    // load config
    GachaPoolManager::Instance().LoadConfig(
        Config::Instance().GetConfigDir() + "gacha_pool.yaml");

    // start logic loop
    // PlayerLogicLoop::Instance().Start();

    // create player
    auto player = PlayerManager::Instance().CreatePlayer(1);

    player->GetCurrency().Add(100000);

    const int TEST_DRAWS = 1000;

    for (int i = 0; i < TEST_DRAWS; i++)
    {
        player->GetCommandQueue().Push([player]()
                                       {
            const int COST = 160;

            if(!player->GetCurrency().Spend(COST))
                return;

            auto item =
                GachaSystem::Instance().DrawOnce(*player);

            player->GetInventory().AddItem(item.id);

            player->GetGachaHistory().Record(item.rarity); });
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto items = player->GetInventory().GetItems();

    std::cout << "Total items: " << items.size() << std::endl;

    // PlayerLogicLoop::Instance().Stop();
}

int main()
{
    // 初始化日志
    Logger::Init();

    // 加载配置
    if (!Config::Instance().Load("config/server.yaml"))
    {
        LOG_ERROR("load config failed");
        return -1;
    }

    // 初始化服务
    ServiceManager::Instance().InitServices();

    GachaPoolManager::Instance().LoadConfig(
        Config::Instance().GetConfigDir() + "gacha_pool.yaml");

    int port = Config::Instance().GetServerPort();

    PlayerLogicLoop::Instance().Start();

    boost::asio::io_context ioContext;

    // 信号处理
    boost::asio::signal_set signals(ioContext, SIGINT, SIGTERM);
    signals.async_wait(
        [&](const boost::system::error_code &, int signal_number)
        {
            LOG_INFO("Capture signal {}, server shutting down...", signal_number);

            ioContext.stop();

            // 可选
            // GlobalThreadPool::Instance().Shutdown();
            PlayerLogicLoop::Instance().Stop();
        });

    // 启动服务器
    TcpServer server(ioContext, port);
    server.StartAccept();

    LOG_INFO("Server started at port {}", port);

    //TestGachaSimulation();

    // 事件循环
    ioContext.run();

    return 0;
}