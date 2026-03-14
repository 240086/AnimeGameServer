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
# 1. 查找系统安装的 GTest
find_package(GTest REQUIRED)
find_package(Protobuf REQUIRED)
find_package(absl REQUIRED CONFIG)
find_package(PkgConfig REQUIRED)

pkg_check_modules(PROTOBUF REQUIRED protobuf)

set(PROTO_DIR "include/network/protocol/proto")
set(PROTO_FILES 
    "${PROTO_DIR}/login.proto"
    "${PROTO_DIR}/gacha.proto"
)

# 3. 自动生成代码
# PROTO_SRCS 会包含生成的 .pb.cc，PROTO_HDRS 会包含 .pb.h
# 生成的文件默认会存放在 build 目录下的相应子目录中
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

include_directories(include)

include_directories(${PROTOBUF_INCLUDE_DIRS})
link_directories(${PROTOBUF_LIBRARY_DIRS})

# 使用 CONFIGURE_DEPENDS 自动感知新文件
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
# 排除 main.cpp
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(AnimeGameServerCore ${SRC_FILES} ${PROTO_SRCS} ${PROTO_HDRS})

target_include_directories(AnimeGameServerCore PUBLIC 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROTOBUF_INCLUDE_DIRS} # 确保 PkgConfig 的头文件路径包含进来
)

target_link_libraries(AnimeGameServerCore
    PUBLIC
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    Boost::system
    ${Protobuf_LIBRARIES}
    
)

add_executable(GameServer src/main.cpp)
target_link_libraries(GameServer 
    PRIVATE 
    AnimeGameServerCore
    ${PROTOBUF_LIBRARIES} # 这里的重复链接是为了确保符号表在最终可执行文件中被正确解析
)

# 找到所有测试相关的 .cpp 文件
file(GLOB_RECURSE TEST_FILES CONFIGURE_DEPENDS tests/*.cpp)

add_executable(TestRunner ${TEST_FILES})
# 2. 将 GTest 链接到测试程序
target_link_libraries(TestRunner 
    PRIVATE 
    AnimeGameServerCore
    GTest::gtest
    # GTest::gtest_main
    ${PROTOBUF_LIBRARIES}
)

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