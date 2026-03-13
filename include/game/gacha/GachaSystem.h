// F:\VSCode_project\Cpp_Proj\AnimeGameServer\include\services\gacha\GachaSystem.h
#pragma once

#include "game/gacha/GachaPool.h"

class Player;
class GachaSystem
{
public:

    static GachaSystem& Instance();

    GachaItem DrawOnce(Player& player);
    std::vector<GachaItem> DrawTen(Player& player);

private:

    GachaSystem();

private:

    GachaPool pool_;

    int default_pool_id_ = 1;
};