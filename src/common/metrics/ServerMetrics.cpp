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