#include "database/player/PlayerLoader.h"
#include "database/mysql/MySQLConnectionPool.h"
#include "common/logger/Logger.h"
#include <vector>

namespace
{
    // 登录时只加载最近窗口，避免单玩家超长历史拖垮数据库与登录时延
    // 1000条通常足以覆盖所有抽卡保底逻辑的计算需求
    constexpr size_t MAX_GACHA_HISTORY_LOAD = 500;
}

bool PlayerLoader::Load(uint64_t playerId, Player &player)
{
    if (!LoadCurrency(playerId, player))
        return false;

    if (!LoadInventory(playerId, player))
        return false;

    if (!LoadGachaHistory(playerId, player))
        return false;

    // 清理加载阶段产生的脏标记，避免登录后立刻触发无意义持久化
    (void)player.FetchDirtyFlags();

    LOG_INFO("Player {} data loaded successfully", playerId);

    return true;
}

bool PlayerLoader::LoadCurrency(uint64_t playerId, Player &player)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();
    if (!conn)
    {
        LOG_ERROR("MySQL connection acquire failed in LoadCurrency");
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
    if (row && row[0])
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
        if (row[0] && row[1])
        {
            int itemId = std::stoi(row[0]);
            int count = std::stoi(row[1]);
            player.GetInventory().AddItem(itemId, count);
        }
    }

    return true;
}

bool PlayerLoader::LoadGachaHistory(uint64_t playerId, Player &player)
{
    auto conn = MySQLConnectionPool::Instance().Acquire();
    if (!conn)
        return false;

    // 🔥 关键修改：使用 DESC + LIMIT 仅获取最新的 1000 条
    std::string sql =
        "SELECT rarity FROM gacha_history "
        "WHERE player_id=" +
        std::to_string(playerId) +
        " ORDER BY id DESC LIMIT " + std::to_string(MAX_GACHA_HISTORY_LOAD);

    auto result = conn->Query(sql);
    if (!result)
    {
        LOG_ERROR("LoadGachaHistory query failed playerId={}", playerId);
        return false;
    }

    std::vector<int> rarities;
    rarities.reserve(MAX_GACHA_HISTORY_LOAD);

    MYSQL_ROW row;
    while ((row = result->FetchRow()) != nullptr)
    {
        if (row[0])
        {
            rarities.push_back(std::stoi(row[0]));
        }
    }

    // 🔥 关键修改：逆序回放（rbegin），将最新的数据按时间正序 Record 进内存
    // 这样内存里的顺序依然是 [旧 -> 新]，符合保底算法逻辑
    for (auto it = rarities.rbegin(); it != rarities.rend(); ++it)
    {
        player.GetGachaHistory().Record(*it);
    }

    const auto &history = player.GetGachaHistory().GetHistory();
    if (!history.empty())
    {
        // 标记已持久化的位置，防止重复存库
        player.GetGachaHistory().MarkPersisted(history.back().seq);
    }

    return true;
}