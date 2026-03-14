#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <boost/asio.hpp>

class AsioContextPool {
public:
    explicit AsioContextPool(size_t size);
    ~AsioContextPool(); // 增加析构函数

    // 禁止拷贝
    AsioContextPool(const AsioContextPool&) = delete;
    AsioContextPool& operator=(const AsioContextPool&) = delete;

    void Run();
    void Stop();
    boost::asio::io_context& GetIOContext();

private:
    using IOContextPtr = std::unique_ptr<boost::asio::io_context>;
    using WorkGuard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

    std::vector<IOContextPtr> contexts_;
    std::vector<WorkGuard> work_guards_; // 使用现代的 WorkGuard
    std::vector<std::thread> threads_;
    std::atomic<size_t> next_{0};        // 原子操作保证线程安全
};