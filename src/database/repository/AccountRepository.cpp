#include "database/repository/AccountRepository.h"
#include "database/mysql/MySQLConnectionPool.h"
#include "database/mysql/MySQLResult.h"

AccountRepository& AccountRepository::Instance()
{
    static AccountRepository instance;
    return instance;
}

std::optional<uint64_t> AccountRepository::GetAccountId(
    const std::string& username,
    const std::string& password)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();

    if (!conn)
        return std::nullopt;

    std::string sql =
        "SELECT player_id FROM accounts "
        "WHERE username='" + username +
        "' AND password='" + password + "' LIMIT 1";

    auto result = conn->Query(sql);

    if (!result)
        return std::nullopt;

    auto row = result->FetchRow();

    if (!row)
        return std::nullopt;

    uint64_t playerId = std::stoull(row[0]);

    return playerId;
}