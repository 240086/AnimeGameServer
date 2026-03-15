#pragma once

#include <mysql/mysql.h>

class MySQLResult
{
public:

    explicit MySQLResult(MYSQL_RES* res)
        : res_(res)
    {}

    ~MySQLResult()
    {
        if (res_)
            mysql_free_result(res_);
    }

    MYSQL_RES* Get()
    {
        return res_;
    }

    MYSQL_ROW FetchRow()
    {
        return mysql_fetch_row(res_);
    }

    unsigned long* FetchLengths()
    {
        return mysql_fetch_lengths(res_);
    }

    unsigned int FieldCount()
    {
        return mysql_num_fields(res_);
    }

private:

    MYSQL_RES* res_ = nullptr;
};