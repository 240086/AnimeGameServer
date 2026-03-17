#include "database/repository/AccountRepository.h"
#include "database/mysql/MySQLConnectionPool.h"
#include "database/mysql/MySQLResult.h"
#include "common/logger/Logger.h"
#include <vector>

namespace
{
    std::string EscapeSqlLiteral(MYSQL *rawConn, const std::string &input)
    {
        if (!rawConn)
            return {};

        std::vector<char> escaped(input.size() * 2 + 1, '\0');
        const auto escapedLen = mysql_real_escape_string(
            rawConn,
            escaped.data(),
            input.c_str(),
            static_cast<unsigned long>(input.size()));

        return std::string(escaped.data(), escapedLen);
    }
} // namespace

AccountRepository &AccountRepository::Instance()
{
    static AccountRepository instance;
    return instance;
}

std::optional<uint64_t> AccountRepository::GetAccountId(
    const std::string &username,
    const std::string &password)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();

    if (!conn)
    {
        LOG_ERROR("GetAccountId failed: acquire mysql connection failed");
        return std::nullopt;
    }

    auto *rawConn = conn->Get();
    if (!rawConn)
    {
        LOG_ERROR("GetAccountId failed: mysql raw connection is null");
        return std::nullopt;
    }

    const auto safeUsername = EscapeSqlLiteral(rawConn, username);
    const auto safePassword = EscapeSqlLiteral(rawConn, password);

    std::string sql =
        "SELECT player_id FROM accounts "
        "WHERE username='" +
        safeUsername +
        "' AND password='" + safePassword + "' LIMIT 1";

    auto result = conn->Query(sql);

    if (!result)
        return std::nullopt;

    auto row = result->FetchRow();

    if (!row)
        return std::nullopt;

    uint64_t playerId = std::stoull(row[0]);

    return playerId;
}