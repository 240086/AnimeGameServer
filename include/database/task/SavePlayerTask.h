#pragma once

#include <memory>
#include <cstdint>

#include "database/task/DatabaseTask.h"

class Player;

class SavePlayerTask : public DatabaseTask
{
public:
    SavePlayerTask(std::shared_ptr<Player> player, uint32_t dirtyFlags);

    void Execute(MySQLConnection *conn) override;

private:
    std::shared_ptr<Player> player_;

    uint32_t dirtyFlags_;
};