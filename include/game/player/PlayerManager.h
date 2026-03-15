#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>

class Player;

class PlayerManager {
public:
    static PlayerManager& Instance();

    // 基础增删改查
    void AddPlayer(std::shared_ptr<Player> player);
    std::shared_ptr<Player> GetPlayer(uint64_t uid);
    void RemovePlayer(uint64_t uid);

    // 统计与批量操作
    size_t OnlineCount();
    
    // 安全遍历：采用分桶遍历，减小锁粒度
    void ForEachPlayer(std::function<void(const std::shared_ptr<Player>&)> func);

private:
    PlayerManager() = default;

    // 使用 64 个桶，支撑万级并发
    static constexpr size_t BUCKET_COUNT = 64;

    struct Bucket {
        std::mutex mutex;
        std::unordered_map<uint64_t, std::shared_ptr<Player>> players;
    };

    Bucket buckets_[BUCKET_COUNT];

    size_t GetBucketIndex(uint64_t id) const { return id % BUCKET_COUNT; }
};