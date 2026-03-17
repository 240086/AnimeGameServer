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

    // 🔥 修正重点：
    // 1. 将 player_id 改为 id (匹配你数据库中的主键名)
    // 2. 将 password 改为 password_hash (匹配你 Python 灌数脚本中的字段名)
    std::string sql =
        "SELECT id FROM accounts "
        "WHERE username='" +
        safeUsername +
        "' AND password_hash='" + safePassword + "' LIMIT 1";

    auto result = conn->Query(sql);

    if (!result)
        return std::nullopt;

    auto row = result->FetchRow();

    if (!row || !row[0]) // 增加 row[0] 的判空保护
        return std::nullopt;

    try
    {
        uint64_t playerId = std::stoull(row[0]);
        return playerId;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("GetAccountId: stoull failed for id={}, error={}", row[0], e.what());
        return std::nullopt;
    }
}