#pragma once

class ServiceManager
{
public:

    static ServiceManager& Instance();

    void InitServices();
};