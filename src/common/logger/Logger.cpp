#include "common/logger/Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Logger::s_Logger;

void Logger::Init()
{
    s_Logger = spdlog::stdout_color_mt("GameServer");

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

    spdlog::set_level(spdlog::level::debug);
}

std::shared_ptr<spdlog::logger>& Logger::GetLogger()
{
    return s_Logger;
}