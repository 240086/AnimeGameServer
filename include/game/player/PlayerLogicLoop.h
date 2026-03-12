#pragma once

#include <thread>
#include <atomic>

class PlayerLogicLoop
{
public:

    static PlayerLogicLoop& Instance();

    void Start();

    void Stop();

private:

    void Loop();

private:

    std::thread thread_;

    std::atomic<bool> running_{false};
};