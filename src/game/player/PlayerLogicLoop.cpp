#include "game/player/PlayerLogicLoop.h"
#include "game/player/PlayerManager.h"

#include <chrono>

PlayerLogicLoop& PlayerLogicLoop::Instance()
{
    static PlayerLogicLoop instance;
    return instance;
}

void PlayerLogicLoop::Start()
{
    running_ = true;

    thread_ = std::thread(&PlayerLogicLoop::Loop, this);
}

void PlayerLogicLoop::Stop()
{
    running_ = false;

    if (thread_.joinable())
        thread_.join();
}

void PlayerLogicLoop::Loop()
{
    while (running_)
    {
        auto& mgr = PlayerManager::Instance();

        for (auto& [id, player] : mgr.GetAllPlayers())
        {
            player->GetCommandQueue().Execute();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}