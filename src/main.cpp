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
#include <boost/asio/steady_timer.hpp>
#include "network/session/SessionManager.h"
#include "database/mysql/MySQLConnectionPool.h"
#include "database/worker/DBWorkerPool.h"
#include "game/player/PlayerManager.h"
#include "database/queue/SaveQueue.h"
#include "database/task/SavePlayerTask.h"
#include "common/metrics/ServerMetrics.h"

int main()
{
    // 1. 初始化基础组件
    Logger::Init();

    if (!Config::Instance().Load("config/server.yaml"))
    {
        LOG_ERROR("load config failed");
        return -1;
    }

    bool dbOk = MySQLConnectionPool::Instance().Init(
        Config::Instance().GetMysqlHost(),
        Config::Instance().GetMysqlPort(),
        Config::Instance().GetMysqlUser(),
        Config::Instance().GetMysqlPassword(),
        Config::Instance().GetMysqlDatabase(),
        Config::Instance().GetMysqlPoolSize());

    if (!dbOk)
    {
        LOG_ERROR("Failed to initialize MySQL Connection Pool");
        return -1;
    }

    size_t cpu = std::max<size_t>(2, std::thread::hardware_concurrency());

    // IO 线程不要硬编码为 CPU/2，优先使用配置项，压测时可快速放大网络处理能力
    size_t ioThreads = std::max<size_t>(
        2,
        static_cast<size_t>(Config::Instance().GetWorkerThreads()));

    size_t logicThreads = cpu;
    LOG_INFO("Thread config: ioThreads={} logicThreads={}", ioThreads, logicThreads);

    // 2. 启动逻辑引擎 (重要修复：必须先启动 ActorSystem)
    // 建议分配 4 个线程处理逻辑，或者根据 CPU 核心数分配
    ActorSystem::Instance().Start(logicThreads);
    LOG_INFO("ActorSystem started with {} worker threads", logicThreads);

    // 3. 初始化业务系统
    ServiceManager::Instance().InitServices();
    GachaPoolManager::Instance().LoadConfig(
        Config::Instance().GetConfigDir() + "gacha_pool.yaml");

    int port = Config::Instance().GetServerPort();

    // 4. 网络基础设施
    boost::asio::io_context mainContext;
    boost::asio::signal_set signals(mainContext, SIGINT, SIGTERM);

    // IO 线程池：负责网络读写与 Protobuf 解析
    AsioContextPool contextPool(ioThreads);

    DBWorkerPool::Instance().Start(SaveQueue::Instance().GetShardCount() * 2);

    LOG_INFO("DBWorkerPool started with {} threads", SaveQueue::Instance().GetShardCount() * 2);

    // 5. 创建服务器
    auto server = std::make_shared<TcpServer>(
        mainContext,
        contextPool,
        port);

    // 6. 注册信号处理逻辑（优雅退出）
    signals.async_wait(
        [&](const boost::system::error_code &ec, int signal_number)
        {
            if (!ec)
            {
                LOG_INFO("Shutdown signal ({}) received. Starting graceful exit...", signal_number);

                LOG_INFO("Flushing all players before shutdown...");

                PlayerManager::Instance().ForEachPlayer(
                    [](const std::shared_ptr<Player> &p)
                    {
                        uint32_t flags = static_cast<uint32_t>(PlayerDirtyFlag::ALL);
                        auto task = std::make_unique<SavePlayerTask>(p, flags);
                        SaveQueue::Instance().Push(p->GetId(), std::move(task));
                    });

                // 停止顺序：先断开连接 -> 停逻辑 -> 停网络池
                server->Stop();
                ActorSystem::Instance().Stop();
                contextPool.Stop();
                mainContext.stop();
            }
        });

    // --- 新增：注册全局心跳超时检查定时器 ---
    std::shared_ptr<boost::asio::steady_timer> timeoutTimer =
        std::make_shared<boost::asio::steady_timer>(mainContext, std::chrono::seconds(30));

    // 定义定时器回调函数（Lambda）
    std::function<void(const boost::system::error_code &)> timerHandler;
    timerHandler = [&](const boost::system::error_code &ec)
    {
        if (!ec)
        {
            LOG_INFO("Running periodic session timeout check...");
            SessionManager::Instance().CheckTimeout();

            // 重新设置定时器，实现循环
            timeoutTimer->expires_after(std::chrono::seconds(30));
            timeoutTimer->async_wait(timerHandler);
        }
    };

    // 启动第一次等待
    timeoutTimer->async_wait(timerHandler);
    // ------------------------------------
    // --- AutoSave 定时器 ---
    std::shared_ptr<boost::asio::steady_timer> autoSaveTimer =
        std::make_shared<boost::asio::steady_timer>(mainContext, std::chrono::seconds(60));

    std::function<void(const boost::system::error_code &)> autoSaveHandler;

    autoSaveHandler = [&](const boost::system::error_code &ec)
    {
        if (!ec)
        {
            LOG_INFO("Running AutoSave Tick...");

            PlayerManager::Instance().OnAutoSaveTick();

            autoSaveTimer->expires_after(std::chrono::seconds(60));
            autoSaveTimer->async_wait(autoSaveHandler);
        }
    };

    autoSaveTimer->async_wait(autoSaveHandler);

    // --- Metrics Timer ---
    std::shared_ptr<boost::asio::steady_timer> metricsTimer =
        std::make_shared<boost::asio::steady_timer>(
            mainContext,
            std::chrono::seconds(5));

    std::function<void(const boost::system::error_code &)> metricsHandler;

    metricsHandler = [&](const boost::system::error_code &ec)
    {
        if (!ec)
        {
            ServerMetrics::Instance().PrintReport();

            metricsTimer->expires_after(std::chrono::seconds(5));
            metricsTimer->async_wait(metricsHandler);
        }
    };

    metricsTimer->async_wait(metricsHandler);

    // 7. 启动网络监听
    server->StartAccept();
    LOG_INFO("AnimeGameServer started at port {}", port);

    // 启动 IO 线程池
    contextPool.Run();

    // 主线程阻塞，驱动 Main Reactor (Acceptor & Signals)
    mainContext.run();

    LOG_INFO("Server exited cleanly.");
    return 0;
}