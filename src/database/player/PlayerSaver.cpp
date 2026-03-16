#include "database/player/PlayerSaver.h"
#include "database/mysql/MySQLConnectionPool.h"

#include "game/player/Player.h"
#include "game/player/Inventory.h"
#include "game/player/GachaHistory.h"
#include "game/player/Currency.h"
#include "common/logger/Logger.h"

bool PlayerSaver::Save(MySQLConnection *conn, std::shared_ptr<Player> player, uint32_t dirtyFlags)
{
    if (!conn || !player)
        return false;

    // 1. 开启事务
    if (!conn->Execute("START TRANSACTION"))
    {
        LOG_ERROR("Failed to start transaction for player {}", player->GetId());
        return false;
    }

    bool allSuccess = true;

    // --- 执行分表保存 ---

    // 基础数据
    if (dirtyFlags & static_cast<uint32_t>(PlayerDirtyFlag::BASE))
    {
        // if (!SaveBaseInfo(conn, *player)) allSuccess = false;
    }

    // 货币
    if (allSuccess && (dirtyFlags & static_cast<uint32_t>(PlayerDirtyFlag::CURRENCY)))
    {
        if (!SaveCurrency(conn, *player))
            allSuccess = false;
    }

    // 背包
    if (allSuccess && (dirtyFlags & static_cast<uint32_t>(PlayerDirtyFlag::INVENTORY)))
    {
        if (!SaveInventory(conn, *player))
            allSuccess = false;
    }

    // 抽卡记录
    if (allSuccess && (dirtyFlags & static_cast<uint32_t>(PlayerDirtyFlag::GACHA_HISTORY)))
    {
        if (!SaveGachaHistory(conn, *player))
            allSuccess = false;
    }

    // 2. 事务终点处理
    if (allSuccess)
    {
        if (conn->Execute("COMMIT"))
        {
            return true; // 真正成功
        }
        else
        {
            LOG_ERROR("Transaction COMMIT failed for player {}", player->GetId());
            conn->Execute("ROLLBACK");
            return false;
        }
    }
    else
    {
        // 任何一步失败，回滚所有操作
        LOG_ERROR("Transaction sub-operation failed, rolling back for player {}", player->GetId());
        conn->Execute("ROLLBACK");
        return false;
    }
}

bool PlayerSaver::SaveCurrency(MySQLConnection *conn, Player &player)
{
    uint64_t amount = player.GetCurrency().Get();
    uint64_t pid = player.GetId();

    // 使用拼接优化
    std::string sql = "INSERT INTO player_currency(player_id,currency_type,amount) VALUES(" + std::to_string(pid) + ",1," + std::to_string(amount) + ") ON DUPLICATE KEY UPDATE amount=" + std::to_string(amount);

    return conn->Execute(sql);
}

bool PlayerSaver::SaveInventory(MySQLConnection *conn, Player &player)
{
    uint64_t pid = player.GetId();

    // 1. 先清空当前玩家的所有背包记录
    std::string clearSql = "DELETE FROM player_inventory WHERE player_id = " + std::to_string(pid);
    if (!conn->Execute(clearSql))
        return false;

    // 2. 如果内存背包为空，直接返回成功（上面已经删干净了）
    const auto &items = player.GetInventory().GetItems();
    if (items.empty())
        return true;

    // 3. 批量插入当前所有物品
    std::string sql = "INSERT INTO player_inventory(player_id, item_id, count) VALUES ";
    bool first = true;
    for (const auto &[itemId, count] : items)
    {
        if (count <= 0)
            continue; // 数量为0的不存
        if (!first)
            sql += ",";
        sql += "(" + std::to_string(pid) + "," + std::to_string(itemId) + "," + std::to_string(count) + ")";
        first = false;
    }

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