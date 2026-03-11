#pragma once

#include "common/thread/ThreadPool.h"

class GlobalThreadPool
{
public:

    static GlobalThreadPool& Instance();

    ThreadPool& GetPool();

private:

    GlobalThreadPool();

    ThreadPool pool_;
};