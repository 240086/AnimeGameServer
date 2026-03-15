#include "common/config/Config.h"

Config &Config::Instance()
{
    static Config instance;
    return instance;
}

bool Config::Load(const std::string &file)
{
    try
    {
        root = YAML::LoadFile(file);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

int Config::GetServerPort() const
{
    return root["server"]["port"].as<int>();
}

int Config::GetWorkerThreads() const
{
    return root["server"]["worker_threads"].as<int>();
}

std::string Config::GetMysqlHost() const
{
    return root["database"]["mysql_host"].as<std::string>();
}

int Config::GetMysqlPort() const
{
    return root["database"]["mysql_port"].as<int>();
}

std::string Config::GetMysqlUser() const
{
    return root["database"]["mysql_user"].as<std::string>("root");
}

std::string Config::GetMysqlPassword() const
{
    return root["database"]["mysql_pwd"].as<std::string>("");
}

std::string Config::GetMysqlDatabase() const
{
    return root["database"]["mysql_db"].as<std::string>("anime_game");
}

int Config::GetMysqlPoolSize() const
{
    return root["database"]["mysql_pool_size"].as<int>(10);
}

std::string Config::GetRedisHost() const
{
    return root["redis"]["host"].as<std::string>();
}

int Config::GetRedisPort() const
{
    return root["redis"]["port"].as<int>();
}

std::string Config::GetConfigDir() const
{
    return "config/";
}