#pragma once

#include <memory>

#include "database/task/DatabaseTask.h"

class Player;

class SavePlayerTask : public DatabaseTask
{
public:

    explicit SavePlayerTask(std::shared_ptr<Player> player);

    void Execute(MySQLConnection* conn) override;

private:

    std::shared_ptr<Player> player_;
};