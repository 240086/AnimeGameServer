#include <gtest/gtest.h>
#include <iostream>

int main(int argc, char **argv)
{
    std::cout << "Starting AnimeGameServer Unit Tests..." << std::endl;

    // 初始化 gtest 环境（解析命令行参数等）
    testing::InitGoogleTest(&argc, argv);

    // 运行所有 TEST() 宏定义的测试用例
    // 它会自动找到你 TestSession.cpp 里的 SessionTest.BindPlayer
    return RUN_ALL_TESTS();
}