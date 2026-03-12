#pragma once

#include <vector>
#include <thread>
#include <atomic>

#include "game/actor/PlayerActor.h"

class ActorSystem
{
public:

    static ActorSystem& Instance();

    void Start(int threads = 4);

    void Register(PlayerActor* actor);

private:

    void Worker();

private:

    std::vector<PlayerActor*> actors_;

    std::vector<std::thread> workers_;

    std::atomic<bool> running_{false};
};