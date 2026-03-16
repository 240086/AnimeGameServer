#include "database/task/SavePlayerTask.h"

#include "database/player/PlayerSaver.h"
#include "game/player/Player.h"

SavePlayerTask::SavePlayerTask(std::shared_ptr<Player> player)
    : player_(player)
{
}

void SavePlayerTask::Execute(MySQLConnection* conn)
{
    PlayerSaver::Save(player_);
}