#include <hiredis/hiredis.h>
#include <string>
#include <memory>
#include <optional>

class RedisClient
{
public:
    RedisClient();
    ~RedisClient();

    // 禁止拷贝 (防止 Context 被重复释放)
    RedisClient(const RedisClient &) = delete;
    RedisClient &operator=(const RedisClient &) = delete;

    bool Connect(const std::string &host, int port);

    // 使用 std::optional 处理 Key 不存在的情况，比返回空字符串更专业
    std::optional<std::string> Get(const std::string &key);
    bool Set(const std::string &key, const std::string &value, int expireSeconds = 0);

private:
    bool CheckConnection(); // 自动重连哨兵

private:
    redisContext *ctx_ = nullptr;
    std::string host_;
    int port_ = 0;
};