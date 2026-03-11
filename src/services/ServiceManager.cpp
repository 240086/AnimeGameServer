#include "services/ServiceManager.h"
#include "services/LoginService.h"
#include "services/GachaService.h"

ServiceManager& ServiceManager::Instance()
{
    static ServiceManager instance;
    return instance;
}

void ServiceManager::InitServices()
{
    LoginService::Instance().Init();
    GachaService::Instance().Init();
}