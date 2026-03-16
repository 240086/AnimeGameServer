#pragma once

#include <thread>
#include <atomic>

class DBWorker
{
public:
    DBWorker(size_t shardIndex); // 构造时指定负责的分片索引

    ~DBWorker();

    void Start();

    void Stop();

private:
    void Run();

    std::thread thread_;

    size_t shardIndex_;

    std::atomic<bool> running_{false};
};