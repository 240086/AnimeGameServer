#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

class Config
{
public:
    static Config &Instance();

    bool Load(const std::string &file);

    int GetServerPort() const;
    int GetWorkerThreads() const;

    std::string GetMysqlHost() const;
    int GetMysqlPort() const;
    std::string GetMysqlUser() const;     // 新增
    std::string GetMysqlPassword() const; // 新增
    std::string GetMysqlDatabase() const; // 新增
    int GetMysqlPoolSize() const;         // 新增

    std::string GetRedisHost() const;
    int GetRedisPort() const;

    std::string GetConfigDir() const;

private:
    Config() = default;

private:
    YAML::Node root;
};