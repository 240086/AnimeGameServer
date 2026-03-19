#pragma once
#include <string>

/**
 * @brief Redis Key 统一管理中心 (工业级规范)
 * 格式规范: [Version]:[Namespace]:[Module]:[Identifier]
 */
class RedisKeyManager
{
public:
    // 全局数据版本号：逻辑变更、结构变更时手动升级 (例如 "v1" -> "v2")
    static constexpr const char *GLOBAL_V = "v1";

    // 命名空间：区分业务大类
    static constexpr const char *NS_PLAYER = "player";
    static constexpr const char *NS_LOCK = "lock";

    /**
     * @brief 核心聚合数据 Key (Hash Slot 友好型)
     * 使用 {id} 语法确保该玩家的所有数据在 Redis Cluster 中落在同一个分片
     */
    static std::string PlayerData(uint64_t playerId)
    {
        return Format(GLOBAL_V, NS_PLAYER, "data", playerId);
    }

    /**
     * @brief 空缓存保护 Key (防止穿透)
     */
    static std::string PlayerNull(uint64_t playerId)
    {
        return Format(GLOBAL_V, NS_PLAYER, "null", playerId);
    }

    /**
     * @brief 加载分布式锁 Key (针对特定玩家的加载锁)
     */
    static std::string PlayerLoadLock(uint64_t playerId)
    {
        return Format(GLOBAL_V, NS_LOCK, "p_load", playerId);
    }

private:
    /**
     * @brief 内部格式化工具
     * 最终生成样式: v1:player:data:{1001}
     */
    static std::string Format(const char *v, const char *ns, const char *mod, uint64_t id)
    {
        std::string s;
        s.reserve(64);
        s.append(v).append(":").append(ns).append(":").append(mod).append(":").append("{").append(std::to_string(id)).append("}"); // {} 用于 Cluster Slot 聚合
        return s;
    }
};