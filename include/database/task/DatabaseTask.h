#pragma once

class MySQLConnection;

class DatabaseTask
{
public:
    virtual ~DatabaseTask() = default;

    virtual void Execute(MySQLConnection* conn) = 0;
};