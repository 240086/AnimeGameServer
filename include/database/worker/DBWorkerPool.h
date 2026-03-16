#pragma once
#include <vector>
#include <memory>
#include <atomic>
#include "database/worker/DBWorker.h"

class DBWorkerPool {
public:
    static DBWorkerPool& Instance();
    ~DBWorkerPool() { Stop(); } // RAII 保证

    void Start(size_t workerCount);
    void Stop();

private:
    DBWorkerPool() = default;
    std::vector<std::unique_ptr<DBWorker>> workers_;
    std::atomic<bool> isStarted_{false};
};