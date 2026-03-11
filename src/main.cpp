#include "common/logger/Logger.h"
#include "common/config/Config.h"
#include "network/TcpServer.h"

#include <boost/asio.hpp>

int main()
{
    Logger::Init();

    if (!Config::Instance().Load("config/server.yaml"))
    {
        LOG_ERROR("load config failed");
        return -1;
    }

    int port = Config::Instance().GetServerPort();

    boost::asio::io_context ioContext;

    TcpServer server(ioContext, port);

    server.StartAccept();

    LOG_INFO("server start at port {}", port);

    ioContext.run();

    return 0;
}