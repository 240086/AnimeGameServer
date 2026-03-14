#include "common/thread/GlobalThreadPool.h"

GlobalThreadPool::GlobalThreadPool()
    : pool_(std::thread::hardware_concurrency()) // 逻辑线程数量
{
}

GlobalThreadPool& GlobalThreadPool::Instance()
{
    static GlobalThreadPool instance;
    return instance;
}

ThreadPool& GlobalThreadPool::GetPool()
{
    return pool_;
}