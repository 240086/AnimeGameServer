#include "game/gacha/GachaSystem.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/protocol/MessageId.h"
#include "common/logger/Logger.h"

GachaSystem::GachaSystem()
{
    pool_.AddItem({1, "5-star character", 5, 0.006});
    pool_.AddItem({2, "4-star character", 4, 0.05});
    pool_.AddItem({3, "3-star weapon", 3, 0.944});
}

GachaSystem& GachaSystem::Instance()
{
    static GachaSystem instance;
    return instance;
}

GachaItem GachaSystem::DrawOnce()
{
    return pool_.Draw();
}
