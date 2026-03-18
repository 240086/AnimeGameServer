#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>

class Player;

class PlayerManager
{
public:
    static PlayerManager &Instance();

    void AddPlayer(std::shared_ptr<Player> player);
    std::shared_ptr<Player> GetPlayer(uint64_t uid);

    // ✅ 修改：返回 player（关键）
    std::shared_ptr<Player> RemovePlayer(uint64_t uid);

    size_t OnlineCount();

    void ForEachPlayer(std::function<void(const std::shared_ptr<Player> &)> func);

    void OnAutoSaveTick();

    // ✅ 新增：异步保存
    void AsyncSavePlayer(std::shared_ptr<Player> player);

private:
    PlayerManager() = default;

    static constexpr size_t BUCKET_COUNT = 64;

    uint64_t autosave_counter_ = 0;

    struct Bucket
    {
        std::mutex mutex;
        std::unordered_map<uint64_t, std::shared_ptr<Player>> players;
    };

    Bucket buckets_[BUCKET_COUNT];

    size_t GetBucketIndex(uint64_t id) const { return id & (BUCKET_COUNT - 1); }
};