cmake_minimum_required(VERSION 3.16)

project(AnimeGameServer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 修复 Boost 警告
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# 1. 查找依赖
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# 2. 头文件目录
include_directories(include)

# 3. 收集源文件
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
)

# 4. 创建核心库（关键）
add_library(AnimeGameServerCore ${SRC_FILES})

target_link_libraries(AnimeGameServerCore
    PUBLIC
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    Boost::system
)

# 5. 服务器可执行程序
add_executable(GameServer src/main.cpp)

target_link_libraries(GameServer
    PRIVATE
    AnimeGameServerCore
)

# 6. 测试程序
add_executable(TestRunner
    tests/TestMain.cpp
    tests/TestGacha.cpp
)

target_link_libraries(TestRunner
    PRIVATE
    AnimeGameServerCore
)

# 7. Windows 特殊处理
if(WIN32)

    target_link_libraries(GameServer PRIVATE ws2_32 mswsock)
    target_link_libraries(TestRunner PRIVATE ws2_32 mswsock)

    target_compile_definitions(GameServer PRIVATE _WIN32_WINNT=0x0601)
    target_compile_definitions(TestRunner PRIVATE _WIN32_WINNT=0x0601)

endif()

# 8. 复制 config
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config DESTINATION ${CMAKE_CURRENT_BINARY_DIR})