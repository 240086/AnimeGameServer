#include "common/logger/Logger.h"
#include "common/config/Config.h"
#include "network/TcpServer.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "services/ServiceManager.h"
#include <boost/asio.hpp>
#include "common/thread/GlobalThreadPool.h"

int main()
{
    Logger::Init();

    if (!Config::Instance().Load("config/server.yaml"))
    {
        LOG_ERROR("load config failed");
        return -1;
    }

    ServiceManager::Instance().InitServices();

    int port = Config::Instance().GetServerPort();

    boost::asio::io_context ioContext;

    TcpServer server(ioContext, port);

    server.StartAccept();

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

    ioContext.run();

    return 0;
}