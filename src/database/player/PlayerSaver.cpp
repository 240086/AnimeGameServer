#include "database/player/PlayerSaver.h"
#include "database/mysql/MySQLConnectionPool.h"

#include "game/player/Player.h"
#include "game/player/Inventory.h"
#include "game/player/GachaHistory.h"
#include "game/player/Currency.h"
#include "common/logger/Logger.h"

bool PlayerSaver::Save(std::shared_ptr<Player> player)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();

    if (!conn)
        return false;

    if (!SaveCurrency(conn.get(), *player))
        return false;

    if (!SaveInventory(conn.get(), *player))
        return false;

    if (!SaveGachaHistory(conn.get(), *player))
        return false;

    return true;
}

bool PlayerSaver::SaveCurrency(MySQLConnection *conn, Player &player)
{
    uint64_t amount = player.GetCurrency().Get();

    std::string sql =
        "INSERT INTO player_currency(player_id,currency_type,amount) VALUES(" +
        std::to_string(player.GetId()) +
        ",1," +
        std::to_string(amount) +
        ") ON DUPLICATE KEY UPDATE amount=" +
        std::to_string(amount);

    return conn->Execute(sql);
}

bool PlayerSaver::SaveInventory(MySQLConnection *conn, Player &player)
{
    const auto &items = player.GetInventory().GetItems();
    if (items.empty())
        return true;

    std::string sql = "INSERT INTO player_inventory(player_id, item_id, count) VALUES ";
    bool first = true;
    for (const auto &[itemId, count] : items)
    {
        if (!first)
            sql += ",";
        sql += "(" + std::to_string(player.GetId()) + "," + std::to_string(itemId) + "," + std::to_string(count) + ")";
        first = false;
    }
    sql += " ON DUPLICATE KEY UPDATE count = VALUES(count);";

    return conn->Execute(sql);
}

bool PlayerSaver::SaveGachaHistory(MySQLConnection *conn, Player &player)
{
    const auto &history = player.GetGachaHistory().GetHistory();

    for (int rarity : history)
    {
        std::string sql =
            "INSERT INTO gacha_history(player_id,item_id,rarity) VALUES(" +
            std::to_string(player.GetId()) + "," +
            "0," +
            std::to_string(rarity) +
            ")";

        if (!conn->Execute(sql))
        {
            LOG_ERROR("Batch save gacha history failed for player {}", player.GetId());
            return false;
        }
    }

    return true;
}