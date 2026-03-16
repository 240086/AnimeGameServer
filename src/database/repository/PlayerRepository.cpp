#include "database/repository/PlayerRepository.h"
#include "database/mysql/MySQLConnectionPool.h"

#include <mysql/mysql.h>
#include "common/logger/Logger.h"

std::shared_ptr<Player> PlayerRepository::LoadPlayer(Player::PlayerId playerId)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();

    if (!conn)
        return nullptr;

    auto player = std::make_shared<Player>(playerId);

    // 1. Load Currency
    {
        std::string sql =
            "SELECT currency FROM players WHERE player_id=" +
            std::to_string(playerId) + " LIMIT 1";

        auto result = conn->Query(sql);

        if (result)
        {
            auto row = result->FetchRow();

            if (row)
            {
                uint64_t currency = std::stoull(row[0]);
                player->GetCurrency().Set(currency);
            }
        }
    }

    // 2. Load Inventory
    {
        std::string sql =
            "SELECT item_id FROM player_items WHERE player_id=" +
            std::to_string(playerId);

        auto result = conn->Query(sql);

        if (result)
        {
            MYSQL_ROW row;

            while ((row = result->FetchRow()))
            {
                int itemId = std::stoi(row[0]);
                player->GetInventory().AddItem(itemId);
            }
        }
    }

    // 3. Load GachaHistory
    {
        std::string sql =
            "SELECT rarity FROM gacha_history WHERE player_id=" +
            std::to_string(playerId) +
            " ORDER BY time ASC";

        auto result = conn->Query(sql);

        if (result)
        {
            MYSQL_ROW row;

            while ((row = result->FetchRow()))
            {
                int rarity = std::stoi(row[0]);
                player->GetGachaHistory().Record(rarity);
            }
        }
    }

    return player;
}

bool PlayerRepository::SaveInventory(Player &player)
{
    // 1. 获取数据库连接
    auto conn = MySQLConnectionPool::Instance().Acquire();
    if (!conn)
    {
        LOG_ERROR("Failed to acquire MySQL connection for player {}", player.GetId());
        return false;
    }

    // 2. 获取背包数据 (假设底层是 std::unordered_map<int, int>)
    const auto &items = player.GetInventory().GetItems();
    if (items.empty())
        return true;

    // 3. 构建批量插入 SQL
    // 使用 ON DUPLICATE KEY UPDATE 确保数量更新而非简单报错
    std::string sql = "INSERT INTO player_inventory (player_id, item_id, count) VALUES ";

    bool first = true;
    for (const auto &[itemId, count] : items)
    {
        if (!first)
            sql += ",";

        sql += "(";
        sql += std::to_string(player.GetId()) + ",";
        sql += std::to_string(itemId) + ",";
        sql += std::to_string(count);
        sql += ")";

        first = false;
    }

    // 如果主键冲突（player_id + item_id），则更新数量列
    sql += " ON DUPLICATE KEY UPDATE count = VALUES(count);";

    // 4. 执行并检查结果
    bool success = conn->Execute(sql);

    if (!success)
    {
        LOG_ERROR("Batch save inventory failed for player {}. SQL: {}", player.GetId(), sql);
    }

    // 5. 归还连接 (如果你的 Pool 是手动归还的话)
    // MySQLConnectionPool::Instance().Release(std::move(conn));

    return success;
}

bool PlayerRepository::SaveCurrency(Player &player)
{
    // 1. 获取数据库连接
    auto conn = MySQLConnectionPool::Instance().Acquire();
    if (!conn)
    {
        LOG_ERROR("Failed to acquire MySQL connection for player {}", player.GetId());
        return false;
    }

    // 2. 获取当前内存中的货币数据
    uint64_t amount = player.GetCurrency().Get();
    uint64_t pid = player.GetId();

    // 3. 构建 SQL 语句
    // 这里假设表中 currency_type 1 代表主货币
    // 使用 ON DUPLICATE KEY UPDATE 确保逻辑的原子性
    std::string sql = "INSERT INTO player_currency (player_id, currency_type, amount) VALUES (" +
                      std::to_string(pid) + ", 1, " + std::to_string(amount) +
                      ") ON DUPLICATE KEY UPDATE amount = " + std::to_string(amount) + ";";

    // 4. 执行并记录结果
    bool success = conn->Execute(sql);
    if (!success)
    {
        LOG_ERROR("Save currency failed for player {}. SQL: {}", pid, sql);
    }

    return success;
}

bool PlayerRepository::InsertGachaRecord(
    Player::PlayerId playerId,
    int itemId,
    int rarity)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();

    std::string sql =
        "INSERT INTO gacha_history(player_id,item_id,rarity) VALUES(" +
        std::to_string(playerId) + "," +
        std::to_string(itemId) + "," +
        std::to_string(rarity) + ")";

    return conn->Execute(sql);
}
