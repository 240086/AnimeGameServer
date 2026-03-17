#pragma once

#include <mysql/mysql.h>
#include <string>
#include <memory>
#include "database/mysql/MySQLResult.h"

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

    std::unique_ptr<MySQLResult> Query(const std::string& sql);

    // 🔥 新增：断线重连机制
    bool Reconnect();
    
    // 🔥 新增：安全关闭资源
    void Close();

private:
    // 内部帮助函数：统一配置超时等选项
    void InitOptions();

private:
    MYSQL* conn_{nullptr};

    // 🔥 新增：保存连接凭据，用于断线后重建连接
    std::string host_;
    int port_{3306};
    std::string user_;
    std::string password_;
    std::string database_;
};