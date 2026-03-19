#include "database/task/SavePlayerTask.h"
#include "database/player/PlayerSaver.h"
#include "game/player/Player.h"
#include "game/player/PlayerDirtyFlag.h"
#include "common/logger/Logger.h"
#include "database/redis/PlayerCache.h"

SavePlayerTask::SavePlayerTask(std::shared_ptr<Player> player, uint32_t dirtyFlags)
    : player_(player),
      dirtyFlags_(dirtyFlags)
{
}

void SavePlayerTask::Execute(MySQLConnection *conn)
{
    // 1. 安全检查
    if (!player_)
    {
        return;
    }

    uint64_t pid = player_->GetId();

    // 2. 使用 try-catch 保护，防止数据库操作抛异常导致状态位死锁
    try
    {
        // 执行保存操作
        bool success = PlayerSaver::Save(conn, player_, dirtyFlags_);

        if (success)
        {
            LOG_INFO("TaskExecutor: Successfully saved player {}", pid);
            if (!PlayerCache::Instance().Save(player_))
            {
                LOG_WARN("TaskExecutor: Player {} saved to MySQL but Redis update FAILED", pid);
            }
        }
        else
        {
            LOG_ERROR("TaskExecutor: FAILED to save player {}. Database error possible.", pid);
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("TaskExecutor: Exception during save for player {}: {}", pid, e.what());
    }
    catch (...)
    {
        LOG_ERROR("TaskExecutor: Unknown error during save for player {}", pid);
    }

    // 3. 🔥 【核心修改】：无论成功、失败还是抛出异常，必须清除保存标志位
    // 这样该 Player 对象才能接受下一次的存盘请求
    player_->ClearSaving();
}