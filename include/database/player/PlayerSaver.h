#pragma once

#include <memory>
#include <cstdint>

class Player;
class MySQLConnection;

class PlayerSaver
{
public:
    static bool Save(MySQLConnection *conn, std::shared_ptr<Player> player, uint32_t dirtyFlags);

private:
    static bool SaveCurrency(MySQLConnection *conn, Player &player);

    static bool SaveInventory(MySQLConnection *conn, Player &player);

    static bool SaveGachaHistory(MySQLConnection *conn, Player &player);
};