#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <atomic>

class Player;

class PlayerManager
{
public:
    static PlayerManager &Instance();

    // --- 生命周期核心接口 ---

    // 登录：处理唯一性冲突，成功后进入内存
    bool Login(uint64_t uid, std::shared_ptr<Player> newPlayer);

    // 登出：移除并强制触发最后一次异步保存
    void Logout(uint64_t uid);

    // 统一获取入口：逻辑层只管调用这个
    std::shared_ptr<Player> GetPlayer(uint64_t uid);

    // --- 保存逻辑 ---
    void AsyncSavePlayer(std::shared_ptr<Player> player, bool forceAll = false);
    void OnAutoSaveTick();

    // --- 工具接口 ---
    size_t OnlineCount();
    void ForEachPlayer(std::function<void(const std::shared_ptr<Player> &)> func);

private:
    PlayerManager() = default;
    ~PlayerManager() = default;

    static constexpr size_t BUCKET_COUNT = 64;
    size_t GetBucketIndex(uint64_t id) const { return id & (BUCKET_COUNT - 1); }

    struct Bucket
    {
        std::mutex mutex;
        std::unordered_map<uint64_t, std::shared_ptr<Player>> players;
    };

    Bucket buckets_[BUCKET_COUNT];
    std::atomic<uint64_t> autosave_counter_{0};
};