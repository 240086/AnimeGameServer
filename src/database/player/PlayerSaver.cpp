#include "database/player/PlayerSaver.h"
#include "game/player/Player.h"
#include "common/logger/Logger.h"
#include <algorithm>
#include "database/mysql/MySQLConnectionPool.h"

bool PlayerSaver::Save(MySQLConnection *conn,
                       std::shared_ptr<Player> player,
                       uint32_t dirtyFlags)
{
    if (!conn || !player)
        return false;

    const uint64_t pid = player->GetId();

    // 1. 显式开启事务，确保 SaveCurrency/Inventory/History 的原子性
    if (!conn->Execute("START TRANSACTION"))
    {
        LOG_ERROR("Failed to start transaction for player {}", pid);
        return false;
    }

    bool success = true;

    // 2. 依次处理脏数据
    if (dirtyFlags & (uint32_t)PlayerDirtyFlag::CURRENCY)
    {
        success = SaveCurrency(conn, *player);
    }

    if (success && (dirtyFlags & (uint32_t)PlayerDirtyFlag::INVENTORY))
    {
        success = SaveInventory(conn, *player);
    }

    if (success && (dirtyFlags & (uint32_t)PlayerDirtyFlag::GACHA_HISTORY))
    {
        success = SaveGachaHistory(conn, *player);
    }

    // 3. 只有全部子任务成功，才提交事务
    if (success)
    {
        if (!conn->Execute("COMMIT"))
        {
            LOG_ERROR("Commit failed for player {}. Data might roll back.", pid);
            conn->Execute("ROLLBACK");
            return false;
        }
        return true;
    }
    else
    {
        // 任何一步失败，全量回滚，保证玩家金币/背包/记录的强一致性
        LOG_WARN("Save failed for player {}, rolling back transaction.", pid);
        conn->Execute("ROLLBACK");
        return false;
    }
}

bool PlayerSaver::SaveCurrency(MySQLConnection *conn, Player &player)
{
    uint64_t pid = player.GetId();
    uint64_t amount = player.GetCurrency().Get();

    std::string sql =
        "INSERT INTO player_currency(player_id, currency_type, amount) VALUES(" +
        std::to_string(pid) + ", 1, " + std::to_string(amount) +
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
    sql.reserve(items.size() * 64);
    sql = "INSERT INTO player_inventory(player_id, item_id, count) VALUES ";

    bool first = true;
    for (const auto &[itemId, count] : items)
    {
        if (count <= 0)
            continue;
        if (!first)
            sql += ",";
        sql += "(" + std::to_string(pid) + "," + std::to_string(itemId) + "," + std::to_string(count) + ")";
        first = false;
    }

    if (first)
        return true;

    sql += " ON DUPLICATE KEY UPDATE count=VALUES(count)";
    return conn->Execute(sql);
}

bool PlayerSaver::SaveGachaHistory(MySQLConnection *conn, Player &player)
{
    auto &gachaComp = player.GetGachaHistory();
    // const auto &history = gachaComp.GetHistory();
    const auto unpersisted = gachaComp.GetUnpersisted();

    // // 🔥 核心改进：获取增量起始点
    // size_t startIndex = gachaComp.GetUnpersisted();
    // if (startIndex >= history.size())
    //     return true;
    if (unpersisted.empty())
        return true;

    uint64_t pid = player.GetId();

    // 🔥 工业级实践：分批写入，防止大包超过 MySQL max_allowed_packet
    constexpr size_t BATCH_SIZE = 100;

    size_t current = 0;
    while (current < unpersisted.size())
    {
        const size_t batchEnd = std::min(current + BATCH_SIZE, unpersisted.size());

        std::string sql;
        sql.reserve(256 + (batchEnd - current) * 48);
        sql = "INSERT INTO gacha_history(player_id, item_id, rarity) VALUES ";

        bool first = true;
        for (size_t i = current; i < batchEnd; ++i)
        {
            if (!first)
                sql += ",";
            sql += "(" + std::to_string(pid) + ",0," + std::to_string(unpersisted[i].rarity) + ")";
            first = false;
        }

        if (!conn->Execute(sql))
        {
            LOG_ERROR("Batch save gacha history failed for player {} at range [{}, {})", pid, current, batchEnd);
            return false; // 退出并触发外层 Rollback
        }

        current = batchEnd;
    }
    gachaComp.MarkPersisted(unpersisted.back().seq);

    return true;
}