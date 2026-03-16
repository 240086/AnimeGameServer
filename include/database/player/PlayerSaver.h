#pragma once

#include <memory>

class Player;
class MySQLConnection;

class PlayerSaver
{
public:

    static bool Save(std::shared_ptr<Player> player);

private:

    static bool SaveCurrency(MySQLConnection* conn, Player& player);

    static bool SaveInventory(MySQLConnection* conn, Player& player);

    static bool SaveGachaHistory(MySQLConnection* conn, Player& player);
};