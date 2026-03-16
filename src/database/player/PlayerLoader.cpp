#include "database/player/PlayerLoader.h"
#include "database/mysql/MySQLConnectionPool.h"
#include "common/logger/Logger.h"

bool PlayerLoader::Load(uint64_t playerId, Player &player)
{
    if (!LoadCurrency(playerId, player))
        return false;

    if (!LoadInventory(playerId, player))
        return false;

    if (!LoadGachaHistory(playerId, player))
        return false;

    LOG_INFO("Player {} data loaded", playerId);

    return true;
}

bool PlayerLoader::LoadCurrency(uint64_t playerId, Player &player)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();

    if (!conn)
    {
        LOG_ERROR("MySQL connection acquire failed");
        return false;
    }

    std::string sql =
        "SELECT amount FROM player_currency "
        "WHERE player_id=" +
        std::to_string(playerId) +
        " AND currency_type=1";

    auto result = conn->Query(sql);

    if (!result)
    {
        LOG_ERROR("LoadCurrency query failed playerId={}", playerId);
        return false;
    }

    auto row = result->FetchRow();

    if (row)
    {
        uint64_t currency = std::stoull(row[0]);
        player.GetCurrency().Set(currency);
    }
    else
    {
        player.GetCurrency().Set(0);
    }

    return true;
}

bool PlayerLoader::LoadInventory(uint64_t playerId, Player &player)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();

    if (!conn)
        return false;

    std::string sql =
        "SELECT item_id, count FROM player_inventory "
        "WHERE player_id=" +
        std::to_string(playerId);

    auto result = conn->Query(sql);

    if (!result)
    {
        LOG_ERROR("LoadInventory query failed playerId={}", playerId);
        return false;
    }

    MYSQL_ROW row;

    while ((row = result->FetchRow()) != nullptr)
    {
        int itemId = std::stoi(row[0]);
        int count = std::stoi(row[1]);

        player.GetInventory().AddItem(itemId, count);
    }

    return true;
}

bool PlayerLoader::LoadGachaHistory(uint64_t playerId, Player &player)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();

    if (!conn)
        return false;

    std::string sql =
        "SELECT rarity FROM gacha_history "
        "WHERE player_id=" +
        std::to_string(playerId) +
        " ORDER BY id ASC";

    auto result = conn->Query(sql);

    if (!result)
    {
        LOG_ERROR("LoadGachaHistory query failed playerId={}", playerId);
        return false;
    }

    MYSQL_ROW row;

    while ((row = result->FetchRow()) != nullptr)
    {
        int rarity = std::stoi(row[0]);

        player.GetGachaHistory().Record(rarity);
    }

    return true;
}