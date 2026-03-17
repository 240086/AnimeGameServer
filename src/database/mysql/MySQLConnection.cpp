#include "database/mysql/MySQLConnection.h"
#include "common/logger/Logger.h"

MySQLConnection::MySQLConnection()
{
    InitOptions();
}

MySQLConnection::~MySQLConnection()
{
    Close();
}

void MySQLConnection::InitOptions()
{
    if (!conn_)
    {
        conn_ = mysql_init(nullptr);
    }

    if (!conn_)
        return;

    // ⚠️ 建议关闭底层自动重连，由应用层连接池统一接管，确保重连后状态（如字符集）一致
    bool reconnect = false;
    mysql_options(conn_, MYSQL_OPT_RECONNECT, &reconnect);

    // 保持你原有的超时防护设置
    unsigned int connectTimeoutSec = 2;
    unsigned int readTimeoutSec = 2;
    unsigned int writeTimeoutSec = 2;
    mysql_options(conn_, MYSQL_OPT_CONNECT_TIMEOUT, &connectTimeoutSec);
    mysql_options(conn_, MYSQL_OPT_READ_TIMEOUT, &readTimeoutSec);
    mysql_options(conn_, MYSQL_OPT_WRITE_TIMEOUT, &writeTimeoutSec);
}

void MySQLConnection::Close()
{
    if (conn_)
    {
        mysql_close(conn_);
        conn_ = nullptr;
    }
}

bool MySQLConnection::Connect(
    const std::string &host,
    int port,
    const std::string &user,
    const std::string &password,
    const std::string &database)
{
    // 🔥 保存凭据以备重连
    host_ = host;
    port_ = port;
    user_ = user;
    password_ = password;
    database_ = database;

    if (!conn_)
    {
        InitOptions();
        if (!conn_)
            return false;
    }

    auto res = mysql_real_connect(
        conn_,
        host_.c_str(),
        user_.c_str(),
        password_.c_str(),
        database_.c_str(),
        port_,
        nullptr,
        0);

    if (!res)
    {
        LOG_ERROR("[MySQL] Connect Failed: %s\n", mysql_error(conn_));
        return false;
    }

    // 设置字符集为 utf8mb4，支持表情符号（动漫游戏必备）
    if (mysql_set_character_set(conn_, "utf8mb4") != 0)
    {
        LOG_ERROR("[MySQL] Set Character Set Failed: %s\n", mysql_error(conn_));
    }

    return true;
}

bool MySQLConnection::Reconnect()
{
    LOG_INFO("[MySQL] Attempting to reconnect...");
    Close();                                                   // 清理旧的失效句柄
    InitOptions();                                             // 重新创建句柄并设置超时属性
    return Connect(host_, port_, user_, password_, database_); // 重新连接并应用字符集
}

MYSQL *MySQLConnection::Get()
{
    return conn_;
}

bool MySQLConnection::Execute(const std::string &sql)
{
    if (mysql_query(conn_, sql.c_str()) != 0)
    {
        LOG_ERROR("[MySQL] Execute Error: %s\n", mysql_error(conn_));
        LOG_ERROR("[MySQL] Failed SQL: %s\n", sql.c_str());
        return false;
    }
    return true;
}

std::unique_ptr<MySQLResult> MySQLConnection::Query(const std::string &sql)
{
    if (mysql_query(conn_, sql.c_str()) != 0)
    {
        LOG_ERROR("[MySQL] Query Error: %s\n", mysql_error(conn_));
        LOG_ERROR("[MySQL] Failed SQL: %s\n", sql.c_str());
        return nullptr;
    }

    MYSQL_RES *res = mysql_store_result(conn_);
    if (!res)
    {
        if (mysql_field_count(conn_) > 0)
        {
            LOG_ERROR("[MySQL] Store Result Error: %s\n", mysql_error(conn_));
        }
        return nullptr;
    }

    return std::make_unique<MySQLResult>(res);
}