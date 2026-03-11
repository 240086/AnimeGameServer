// F:\VSCode_project\Cpp_Proj\AnimeGameServer\include\services\gacha\GachaSystem.h
#pragma once

#include "game/gacha/GachaPool.h"

class GachaSystem
{
public:

    static GachaSystem& Instance();

    GachaItem DrawOnce();

private:

    GachaSystem();

private:

    GachaPool pool_;
};