#include "database/player/PlayerSaver.h"
#include "database/mysql/MySQLConnectionPool.h"

#include "game/player/Player.h"
#include "game/player/Inventory.h"
#include "game/player/GachaHistory.h"
#include "game/player/Currency.h"
#include "common/logger/Logger.h"

bool PlayerSaver::Save(MySQLConnection *conn,
                       std::shared_ptr<Player> player,
                       uint32_t dirtyFlags)
{
    if (!conn || !player)
        return false;

    uint64_t pid = player->GetId();

    if (!conn->Execute("START TRANSACTION"))
    {
        LOG_ERROR("Failed to start transaction for player {}", pid);
        return false;
    }

    bool success = true;

    if (dirtyFlags & (uint32_t)PlayerDirtyFlag::CURRENCY)
    {
        success &= SaveCurrency(conn, *player);
    }

    if (success && (dirtyFlags & (uint32_t)PlayerDirtyFlag::INVENTORY))
    {
        success &= SaveInventory(conn, *player);
    }

    if (success && (dirtyFlags & (uint32_t)PlayerDirtyFlag::GACHA_HISTORY))
    {
        success &= SaveGachaHistory(conn, *player);
    }

    if (success)
    {
        if (!conn->Execute("COMMIT"))
        {
            LOG_ERROR("Commit failed player {}", pid);
            conn->Execute("ROLLBACK");
            return false;
        }
        return true;
    }
    else
    {
        conn->Execute("ROLLBACK");
        return false;
    }
}

bool PlayerSaver::SaveCurrency(MySQLConnection *conn, Player &player)
{
    uint64_t pid = player.GetId();
    uint64_t amount = player.GetCurrency().Get();

    std::string sql =
        "INSERT INTO player_currency(player_id,currency_type,amount) VALUES(" +
        std::to_string(pid) + ",1," +
        std::to_string(amount) +
        ") ON DUPLICATE KEY UPDATE amount=VALUES(amount)";

    return conn->Execute(sql);
}

bool PlayerSaver::SaveInventory(MySQLConnection *conn, Player &player)
{
    const auto &items = player.GetInventory().GetItems();
    if (items.empty())
        return true;

    uint64_t pid = player.GetId();

    std::string sql;
    sql.reserve(items.size() * 40);

    sql = "INSERT INTO player_inventory(player_id,item_id,count) VALUES ";

    bool first = true;

    for (const auto &[itemId, count] : items)
    {
        if (count <= 0)
            continue;

        if (!first)
            sql += ",";

        sql += "(" +
               std::to_string(pid) + "," +
               std::to_string(itemId) + "," +
               std::to_string(count) + ")";

        first = false;
    }

    sql += " ON DUPLICATE KEY UPDATE count=VALUES(count)";

    return conn->Execute(sql);
}

bool PlayerSaver::SaveGachaHistory(MySQLConnection *conn, Player &player)
{
    const auto &history = player.GetGachaHistory().GetHistory();
    if (history.empty())
        return true;

    // 【核心性能优化】：由单条执行改为批量 SQL
    std::string sql = "INSERT INTO gacha_history(player_id, item_id, rarity) VALUES ";
    bool first = true;
    uint64_t pid = player.GetId();

    for (int rarity : history)
    {
        if (!first)
            sql += ",";
        // 假设 itemId 为 0 表示抽卡结果的具体 ID 在此处忽略
        sql += "(" + std::to_string(pid) + ",0," + std::to_string(rarity) + ")";
        first = false;
    }

    if (!conn->Execute(sql))
    {
        LOG_ERROR("Batch save gacha history failed for player {}", pid);
        return false;
    }

    return true;
}