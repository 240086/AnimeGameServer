#include "services/LoginService.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "common/logger/Logger.h"

LoginService& LoginService::Instance()
{
    static LoginService instance;
    return instance;
}

void LoginService::Init()
{
    MessageDispatcher::Instance().RegisterHandler(
        MSG_LOGIN,
        [this](Connection* conn, const char* data, size_t len)
        {
            HandleLogin(conn, data, len);
        });
}

void LoginService::HandleLogin(Connection* conn, const char* data, size_t len)
{
    LOG_INFO("login request received size={}", len);

    // TODO
    // 验证账号
    // 查询数据库
    // 返回登录结果
}