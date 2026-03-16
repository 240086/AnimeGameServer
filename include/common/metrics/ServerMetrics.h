#pragma once

#include <atomic>
#include <cstdint>

class ServerMetrics
{
public:
    static ServerMetrics &Instance();

    // Session
    void IncSession();
    void DecSession();

    // DB Task
    void IncDBTask();
    void IncDBTaskFinished();

    // DB Latency
    void AddDBLatency(uint64_t microseconds);

    // 打印
    void PrintReport();

private:
    ServerMetrics() = default;

private:
    // Session
    std::atomic<uint64_t> sessionCount_{0};

    // DB
    std::atomic<uint64_t> dbTaskTotal_{0};
    std::atomic<uint64_t> dbTaskFinished_{0};

    std::atomic<uint64_t> dbLatencyTotal_{0};
};