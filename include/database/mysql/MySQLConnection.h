#pragma once

#include <mysql/mysql.h>
#include <string>

class MySQLConnection
{
public:
    MySQLConnection();
    ~MySQLConnection();

    bool Connect(
        const std::string& host,
        int port,
        const std::string& user,
        const std::string& password,
        const std::string& database);

    MYSQL* Get();

    bool Execute(const std::string& sql);

    MYSQL_RES* Query(const std::string& sql);

private:
    MYSQL* conn_;
};