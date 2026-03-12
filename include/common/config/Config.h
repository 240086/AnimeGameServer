#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

class Config
{
public:
    static Config& Instance();

    bool Load(const std::string& file);

    int GetServerPort() const;
    int GetWorkerThreads() const;

    std::string GetMysqlHost() const;
    int GetMysqlPort() const;

    std::string GetRedisHost() const;
    int GetRedisPort() const;

    std::string GetConfigDir() const;

private:
    Config() = default;

private:
    YAML::Node root;
};