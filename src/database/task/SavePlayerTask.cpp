#include "database/task/SavePlayerTask.h"

#include "database/player/PlayerSaver.h"
#include "game/player/Player.h"
#include "game/player/PlayerDirtyFlag.h"
#include "common/logger/Logger.h"

SavePlayerTask::SavePlayerTask(std::shared_ptr<Player> player, uint32_t dirtyFlags)
    : player_(player),
      dirtyFlags_(dirtyFlags)
{
}

void SavePlayerTask::Execute(MySQLConnection *conn)
{
    if (!player_)
        return;

    uint64_t pid = player_->GetId();

    // 1. 【新增日志】：追踪任务开始执行的时刻和原因
    LOG_INFO("TaskExecutor: Starting save for player {} [Flags: {}]", pid, dirtyFlags_);

    // 2. 执行保存操作
    // 建议：PlayerSaver::Save 应该返回 bool，以便我们知道是否存盘成功
    bool success = PlayerSaver::Save(conn, player_, dirtyFlags_);

    // 3. 【增强优化】：根据结果记录日志
    if (success)
    {
        LOG_INFO("TaskExecutor: Successfully saved player {}", pid);
    }
    else
    {
        // 如果失败，一定要打印 ERROR 级别，这涉及数据资产安全
        LOG_ERROR("TaskExecutor: FAILED to save player {}. Database error possible.", pid);
    }
}