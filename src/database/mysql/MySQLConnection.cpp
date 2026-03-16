#include "database/mysql/MySQLConnection.h"
#include "common/logger/Logger.h"

MySQLConnection::MySQLConnection()
{
    conn_ = mysql_init(nullptr);
    // 设置自动重连选项，防止空闲连接被 MySQL Server 断开
    bool reconnect = true;
    mysql_options(conn_, MYSQL_OPT_RECONNECT, &reconnect);
}

MySQLConnection::~MySQLConnection()
{
    if (conn_)
        mysql_close(conn_);
}

bool MySQLConnection::Connect(
    const std::string &host,
    int port,
    const std::string &user,
    const std::string &password,
    const std::string &database)
{
    auto res = mysql_real_connect(
        conn_,
        host.c_str(),
        user.c_str(),
        password.c_str(),
        database.c_str(),
        port,
        nullptr,
        0);

    if (!res)
    {
        // 关键：连接失败必须记录原因
        LOG_ERROR("[MySQL] Connect Failed: %s\n", mysql_error(conn_));
        return false;
    }

    // 设置字符集为 utf8mb4，支持表情符号（动漫游戏必备）
    mysql_set_character_set(conn_, "utf8mb4");
    return true;
}

MYSQL *MySQLConnection::Get()
{
    return conn_;
}

bool MySQLConnection::Execute(const std::string &sql)
{
    if (mysql_query(conn_, sql.c_str()) != 0)
    {
        // 关键：记录 SQL 语法错误或连接丢失原因
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
        // 只有在真的出错了才返回空（有些查询可能本身不产生结果集）
        if (mysql_field_count(conn_) > 0)
        {
            LOG_ERROR("[MySQL] Store Result Error: %s\n", mysql_error(conn_));
        }
        return nullptr;
    }

    return std::make_unique<MySQLResult>(res);
}