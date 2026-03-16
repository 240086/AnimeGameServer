#include "database/task/SavePlayerTask.h"

#include "database/player/PlayerSaver.h"
#include "game/player/Player.h"
#include "game/player/PlayerDirtyFlag.h"

SavePlayerTask::SavePlayerTask(std::shared_ptr<Player> player, uint32_t dirtyFlags)
    : player_(player),
      dirtyFlags_(dirtyFlags)
{
}

void SavePlayerTask::Execute(MySQLConnection *conn)
{
    if (!player_)
        return;

    PlayerSaver::Save(conn, player_, dirtyFlags_);
}