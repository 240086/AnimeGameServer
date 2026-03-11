cmake_minimum_required(VERSION 3.16)

project(AnimeGameServer)

# 加上这一行
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 修复 Boost 警告
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# 1. 查找依赖包
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# 2. 包含头文件目录
include_directories(include)

# 3. 递归获取源文件
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
)

# 4. 生成可执行文件
add_executable(GameServer ${SRC_FILES})

# 5. 链接库
target_link_libraries(GameServer
    PRIVATE
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    Boost::system
)

# 6. 【关键修复】针对 Windows 环境的特殊处理
if(WIN32)
    # 链接 Windows 网络库以修复 undefined reference
    target_link_libraries(GameServer PRIVATE ws2_32 mswsock)
    
    # 告诉 Boost 定义 Windows 版本（防止部分 Asio 功能失效）
    target_compile_definitions(GameServer PRIVATE _WIN32_WINNT=0x0601)
endif()

# 7. 【关键修复】将 config 目录拷贝到构建目录
# 这样程序运行时 Load("config/server.yaml") 就能直接找到文件了
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config DESTINATION ${CMAKE_CURRENT_BINARY_DIR})