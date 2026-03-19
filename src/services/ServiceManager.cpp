#include "services/ServiceManager.h"
#include "services/LoginService.h"
#include "services/GachaService.h"
#include "network/protocol/ProtocolRegistry.cpp"

ServiceManager &ServiceManager::Instance()
{
    static ServiceManager instance;
    return instance;
}

void ServiceManager::InitServices()
{
    // 1. 🔥 第一步：协议字典初始化 (ID -> PB Type)
    // 这一步决定了 MessageDecoder 遇到某个 ID 时知道怎么解析
    ProtocolRegistry::RegisterAll();

    // 2. 🔥 第二步：业务分发初始化 (ID -> Callback)
    // 这一步决定了 Dispatcher 遇到某个 ID 时交给哪个 Service 处理
    LoginService::Instance().Init();
    GachaService::Instance().Init();
    
    LOG_INFO("ServiceManager: All services and protocols initialized.");
}