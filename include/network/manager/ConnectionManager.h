#pragma once
#include <unordered_map>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

class Connection;

class ConnectionManager {
public:
    static ConnectionManager &Instance();

    int AddConnection(std::shared_ptr<Connection> conn);
    void RemoveConnection(int id);
    std::shared_ptr<Connection> GetConnection(int id);
    size_t OnlineCount();

private:
    ConnectionManager(); // 构造函数需要初始化桶

    // 定义桶结构
    struct Bucket {
        std::unordered_map<int, std::shared_ptr<Connection>> connections;
        mutable std::mutex mutex; // 每个桶一把简单的互斥锁即可，竞争极低
    };

    static constexpr size_t BUCKET_COUNT = 64; // 建议设为 2 的幂，方便取模
    std::vector<Bucket> buckets_;
    std::atomic<int> next_id_{1};
};