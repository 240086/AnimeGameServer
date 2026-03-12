#include "game/actor/ActorSystem.h"

#include <chrono>

ActorSystem& ActorSystem::Instance()
{
    static ActorSystem instance;
    return instance;
}

void ActorSystem::Start(int threads)
{
    running_ = true;

    for(int i=0;i<threads;i++)
    {
        workers_.emplace_back(&ActorSystem::Worker,this);
    }
}

void ActorSystem::Register(PlayerActor* actor)
{
    actors_.push_back(actor);
}

void ActorSystem::Worker()
{
    while(running_)
    {
        for(auto actor : actors_)
        {
            actor->Process();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}