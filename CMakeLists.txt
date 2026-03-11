cmake_minimum_required(VERSION 3.16)

project(AnimeGameServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)

file(GLOB_RECURSE SRC_FILES
    src/*.cpp
)

add_executable(GameServer ${SRC_FILES})

target_link_libraries(GameServer
    spdlog::spdlog
    yaml-cpp
)

# 在 CMakeLists.txt 末尾添加
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config DESTINATION ${CMAKE_CURRENT_BINARY_DIR})