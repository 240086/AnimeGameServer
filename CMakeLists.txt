cmake_minimum_required(VERSION 3.16)
project(AnimeGameServer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

include_directories(include)

# 使用 CONFIGURE_DEPENDS 自动感知新文件
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
# 排除 main.cpp
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(AnimeGameServerCore ${SRC_FILES})

target_link_libraries(AnimeGameServerCore
    PUBLIC
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    Boost::system
)

add_executable(GameServer src/main.cpp)
target_link_libraries(GameServer PRIVATE AnimeGameServerCore)

add_executable(TestRunner tests/TestMain.cpp tests/TestGacha.cpp)
target_link_libraries(TestRunner PRIVATE AnimeGameServerCore)

if(WIN32)
    target_link_libraries(GameServer PRIVATE ws2_32 mswsock)
    target_link_libraries(TestRunner PRIVATE ws2_32 mswsock)
    target_compile_definitions(GameServer PRIVATE _WIN32_WINNT=0x0601)
    target_compile_definitions(TestRunner PRIVATE _WIN32_WINNT=0x0601)
endif()

# 每次编译都确保 config 是最新的
add_custom_command(TARGET AnimeGameServerCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/config ${CMAKE_CURRENT_BINARY_DIR}/config
)