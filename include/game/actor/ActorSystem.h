#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

class Actor;

class ActorSystem
{
public:
    static ActorSystem &Instance();

    void Start(int threads);

    void Stop();

    void Schedule(std::shared_ptr<Actor> actor);

private:
    void Worker();

private:
    std::vector<std::thread> workers_;

    std::queue<std::shared_ptr<Actor>> ready_queue_;

    std::mutex mutex_;

    std::condition_variable cond_;

    bool running_ = false;
};