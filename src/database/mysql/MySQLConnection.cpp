#include "database/mysql/MySQLConnection.h"

MySQLConnection::MySQLConnection()
{
    conn_ = mysql_init(nullptr);
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
    return mysql_real_connect(
        conn_,
        host.c_str(),
        user.c_str(),
        password.c_str(),
        database.c_str(),
        port,
        nullptr,
        0);
}

MYSQL *MySQLConnection::Get()
{
    return conn_;
}

bool MySQLConnection::Execute(const std::string &sql)
{
    return mysql_query(conn_, sql.c_str()) == 0;
}

MYSQL_RES *MySQLConnection::Query(const std::string &sql)
{
    if (mysql_query(conn_, sql.c_str()) != 0)
        return nullptr;

    return mysql_store_result(conn_);
}