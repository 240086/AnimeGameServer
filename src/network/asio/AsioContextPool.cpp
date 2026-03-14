#include "network/asio/AsioContextPool.h"

AsioContextPool::AsioContextPool(size_t size) {
    for (size_t i = 0; i < size; ++i) {
        auto ctx = std::make_unique<boost::asio::io_context>();
        // 使用 make_work_guard 防止 context.run() 在没有任务时立即退出
        work_guards_.emplace_back(boost::asio::make_work_guard(*ctx));
        contexts_.push_back(std::move(ctx));
    }
}

AsioContextPool::~AsioContextPool() {
    Stop();
}

void AsioContextPool::Run() {
    for (auto& ctx : contexts_) {
        // 显式获取原始指针，确保线程捕获的是正确的地址
        boost::asio::io_context* raw_ctx = ctx.get();
        threads_.emplace_back([raw_ctx]() {
            raw_ctx->run();
        });
    }
}

void AsioContextPool::Stop() {
    // 1. 停止所有 guard，允许 run() 在任务完成后自然结束
    for (auto& guard : work_guards_) {
        guard.reset();
    }

    // 2. 显式停止所有 context
    for (auto& ctx : contexts_) {
        ctx->stop();
    }

    // 3. 等待所有线程回收
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads_.clear();
}

boost::asio::io_context& AsioContextPool::GetIOContext() {
    // 使用原子操作进行轮询分配
    size_t index = next_.fetch_add(1) % contexts_.size();
    return *contexts_[index];
}