#include "common/logger/Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h> // 控制台彩色输出
#include <spdlog/sinks/daily_file_sink.h>    // 每日滚动文件
#include <spdlog/async.h>                    // 异步支持（必须包含！）
#include <filesystem>

std::shared_ptr<spdlog::logger> Logger::s_Logger;

void Logger::Init()
{
    try
    {

        std::filesystem::path log_path("logs");
        if (!std::filesystem::exists(log_path))
        {
            std::printf("FATAL: Logger Init Failed \n");
            std::filesystem::create_directories(log_path);
        }
        // 1. 初始化异步线程池（全局只需要一次）
        spdlog::init_thread_pool(8192, 1);

        // 2. 创建不同的输出目的地 (Sinks)
        // 控制台输出
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);

        // 文件输出（异步模式通过 factory 实现，或者手动包装 sink）
        auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/server_log.txt", 0, 0);
        file_sink->set_level(spdlog::level::info);

        // 3. 将所有 Sinks 组合在一起
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

        // 4. 创建异步 Logger
        s_Logger = std::make_shared<spdlog::async_logger>(
            "GameServer",
            sinks.begin(),
            sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block // 如果队列满了，阻塞等待（保证日志不丢）
        );

        // 5. 设置全局参数
        s_Logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
        s_Logger->set_level(spdlog::level::debug);

        // 注册为全局默认 logger (可选)
        spdlog::set_default_logger(s_Logger);

        s_Logger->flush_on(spdlog::level::warn);      // 遇到 Warn 及以上级别立刻刷盘
        spdlog::flush_every(std::chrono::seconds(3)); // 每 3 秒自动刷一次盘
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::printf("FATAL: Logger Init Failed: %s\n", ex.what());
    }
}

std::shared_ptr<spdlog::logger> &Logger::GetLogger()
{
    return s_Logger;
}