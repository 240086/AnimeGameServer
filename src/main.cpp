#include "common/logger/Logger.h"
#include "common/config/Config.h"

int main()
{
    Logger::Init();

    if (!Config::Instance().Load("config/server.yaml"))
    {
        LOG_ERROR("load config failed");
        return -1;
    }

    LOG_INFO("server port: {}", Config::Instance().GetServerPort());
    LOG_INFO("worker threads: {}", Config::Instance().GetWorkerThreads());

    LOG_INFO("server start success");

    return 0;
}