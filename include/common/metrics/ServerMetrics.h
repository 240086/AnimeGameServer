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

    // Gacha
    void IncGachaRequest();
    void IncGachaSingleRequest();
    void IncGachaTenRequest();
    void IncGachaSuccess();
    void IncGachaInsufficientFunds();
    void IncGachaInvalidRequest();
    void IncGachaInternalError();

    // Idempotency
    void IncIdemFirstTime();
    void IncIdemInProgress();
    void IncIdemReplay();
    void IncIdemRedisDegrade();

    // 打印
    void PrintReport();

private:
    ServerMetrics() = default;

private:
    // Session
    std::atomic<uint32_t> sessionCount_{0};

    // DB
    std::atomic<uint64_t> dbTaskTotal_{0};
    std::atomic<uint64_t> dbTaskFinished_{0};

    std::atomic<uint64_t> dbLatencyTotal_{0};

    // Gacha
    std::atomic<uint64_t> gachaRequestTotal_{0};
    std::atomic<uint64_t> gachaSingleRequest_{0};
    std::atomic<uint64_t> gachaTenRequest_{0};
    std::atomic<uint64_t> gachaSuccess_{0};
    std::atomic<uint64_t> gachaInsufficientFunds_{0};
    std::atomic<uint64_t> gachaInvalidRequest_{0};
    std::atomic<uint64_t> gachaInternalError_{0};

    // Idempotency
    std::atomic<uint64_t> idemFirstTime_{0};
    std::atomic<uint64_t> idemInProgress_{0};
    std::atomic<uint64_t> idemReplay_{0};
    std::atomic<uint64_t> idemRedisDegrade_{0};
};