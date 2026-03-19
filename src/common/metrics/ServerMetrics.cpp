#include "common/metrics/ServerMetrics.h"
#include "database/queue/SaveQueue.h"
#include "common/logger/Logger.h"

ServerMetrics &ServerMetrics::Instance()
{
    static ServerMetrics instance;
    return instance;
}

void ServerMetrics::IncSession()
{
    sessionCount_++;
}

void ServerMetrics::DecSession()
{
    sessionCount_--;
}

void ServerMetrics::IncDBTask()
{
    dbTaskTotal_++;
}

void ServerMetrics::IncDBTaskFinished()
{
    dbTaskFinished_++;
}

void ServerMetrics::AddDBLatency(uint64_t us)
{
    dbLatencyTotal_ += us;
}

void ServerMetrics::IncGachaRequest()
{
    gachaRequestTotal_++;
}

void ServerMetrics::IncGachaSingleRequest()
{
    gachaSingleRequest_++;
}

void ServerMetrics::IncGachaTenRequest()
{
    gachaTenRequest_++;
}

void ServerMetrics::IncGachaSuccess()
{
    gachaSuccess_++;
}

void ServerMetrics::IncGachaInsufficientFunds()
{
    gachaInsufficientFunds_++;
}

void ServerMetrics::IncGachaInvalidRequest()
{
    gachaInvalidRequest_++;
}

void ServerMetrics::IncGachaInternalError()
{
    gachaInternalError_++;
}

void ServerMetrics::IncIdemFirstTime()
{
    idemFirstTime_++;
}

void ServerMetrics::IncIdemInProgress()
{
    idemInProgress_++;
}

void ServerMetrics::IncIdemReplay()
{
    idemReplay_++;
}

void ServerMetrics::IncIdemRedisDegrade()
{
    idemRedisDegrade_++;
}

void ServerMetrics::PrintReport()
{
    uint64_t total = dbTaskTotal_.load();
    uint64_t finished = dbTaskFinished_.load();

    uint64_t latency = dbLatencyTotal_.load();

    double avgLatency = 0;
    if (finished > 0)
        avgLatency = (double)latency / finished / 1000.0;

    LOG_INFO("=========== SERVER METRICS ===========");
    LOG_INFO("SessionCount: {}", sessionCount_.load());
    LOG_INFO("DB Tasks: {} finished / {}", finished, total);
    LOG_INFO("DB Avg Latency: {:.3f} ms", avgLatency);
    LOG_INFO("Gacha: total={} (single={}, ten={}), success={}, insufficient_funds={}, invalid_req={}, internal_err={}",
             gachaRequestTotal_.load(),
             gachaSingleRequest_.load(),
             gachaTenRequest_.load(),
             gachaSuccess_.load(),
             gachaInsufficientFunds_.load(),
             gachaInvalidRequest_.load(),
             gachaInternalError_.load());
    LOG_INFO("Idempotency: first_time={}, in_progress={}, replay={}, redis_degrade={}",
             idemFirstTime_.load(),
             idemInProgress_.load(),
             idemReplay_.load(),
             idemRedisDegrade_.load());

    size_t totalBacklog = 0;

    for (size_t i = 0; i < SaveQueue::Instance().GetShardCount(); i++)
    {
        size_t size = SaveQueue::Instance().GetShardQueueSize(i);
        totalBacklog += size;
        if (size > 0)
        {
            LOG_INFO("SaveQueue Shard {} backlog {}", i, size);
        }
    }
    LOG_INFO("SaveQueue Total Backlog: {}", totalBacklog);
    LOG_INFO("======================================");
}