#pragma once

class BaseService
{
public:
    virtual ~BaseService() = default;

    virtual void Init() = 0;
};