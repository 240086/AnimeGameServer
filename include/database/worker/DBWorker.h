#pragma once

#include <thread>
#include <atomic>

class DBWorker
{
public:

    DBWorker();

    ~DBWorker();

    void Start();

    void Stop();

private:

    void Run();

    std::thread thread_;

    std::atomic<bool> running_{false};
};