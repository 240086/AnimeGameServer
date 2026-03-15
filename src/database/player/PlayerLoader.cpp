#include "database/player/PlayerLoader.h"
#include "database/mysql/MySQLConnectionPool.h"
#include "common/logger/Logger.h"

bool PlayerLoader::Load(uint64_t playerId, Player &player)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();

    if (!conn)
    {
        LOG_ERROR("MySQL connection acquire failed");
        return false;
    }

    /* -------- currency -------- */

    {
        std::string sql =
            "SELECT amount FROM player_currency "
            "WHERE player_id=" +
            std::to_string(playerId) +
            " AND currency_type=1";

        auto result = conn->Query(sql);
        player.GetCurrency().Set(0);

        if (!result)
        {
            LOG_ERROR("Load currency failed for player {}", playerId);
            return false;
        }

        if (result)
        {
            auto row = result->FetchRow();

            if (row)
            {
                uint64_t currency = std::stoull(row[0]);
                player.GetCurrency().Set(currency);
            }
        }
    }

    return true;
}