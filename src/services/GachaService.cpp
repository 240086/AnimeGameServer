#include "services/GachaService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "common/logger/Logger.h"

GachaService& GachaService::Instance()
{
    static GachaService instance;
    return instance;
}

void GachaService::Init()
{
    MessageDispatcher::Instance().RegisterHandler(
        MSG_GACHA,
        [this](Connection* conn, const char* data, size_t len)
        {
            HandleGacha(conn, data, len);
        });
}

void GachaService::HandleGacha(Connection* conn, const char* data, size_t len)
{
    LOG_INFO("gacha request received");

    // TODO
    // 抽卡逻辑
}