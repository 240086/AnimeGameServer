#include "common/logger/Logger.h"
#include "common/config/Config.h"

#include "network/TcpServer.h"
#include "network/dispatcher/MessageDispatcher.h"

#include "services/ServiceManager.h"

#include <boost/asio.hpp>

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

    MessageDispatcher::Instance().Dispatch(
        1,
        nullptr,
        "test_login",
        10
    );

    ioContext.run();

    return 0;
}