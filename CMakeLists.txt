cmake_minimum_required(VERSION 3.16)
project(AnimeGameServer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# --- 1. 引入子模块 AnimeCore ---
# 确保在查找 package 之前引入，因为 AnimeCore 可能会定义一些宏或路径
add_subdirectory(third_party/AnimeCore)

# --- 2. 查找依赖 ---
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(GTest REQUIRED)
find_package(Protobuf REQUIRED)
find_package(absl REQUIRED CONFIG)
find_package(PkgConfig REQUIRED)
find_package(nlohmann_json REQUIRED)

pkg_check_modules(PROTOBUF REQUIRED protobuf)
pkg_check_modules(MARIADB REQUIRED libmariadb)
pkg_check_modules(HIREDIS REQUIRED hiredis)

# --- 3. Protobuf 配置 ---
set(PROTO_DIR "include/network/protocol/proto")
set(PROTO_FILES
    "${PROTO_DIR}/login.proto"
    "${PROTO_DIR}/gacha.proto"
    "${PROTO_DIR}/heartbeat.proto"
    "${PROTO_DIR}/common.proto"
)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# --- 4. 源文件管理与“去重”逻辑 ---
file(GLOB_RECURSE ALL_SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

# 【核心重构点】从 SRC_FILES 中剔除掉已由 AnimeCore 实现的底层模块
# 使用正则表达式匹配并移除 common 和 network 底层的冗余实现
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "src/common/config/.*")
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "src/common/logger/.*")
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "src/common/thread/.*")
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "src/network/asio/.*")
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "src/network/buffer/.*")
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "src/network/TcpServer\\.cpp")
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "src/network/Connection\\.cpp")
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "src/network/protocol/Packet.*")

# 移除 main.cpp 以便构建库
set(CORE_SRC_FILES ${ALL_SRC_FILES})
list(REMOVE_ITEM CORE_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

# --- 5. 构建业务核心库 ---
add_library(AnimeGameServerCore ${CORE_SRC_FILES} ${PROTO_SRCS} ${PROTO_HDRS})

# 包含目录优先级：自身 include -> AnimeCore include -> 依赖 include
target_include_directories(AnimeGameServerCore PUBLIC
    include
    third_party/AnimeCore/include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
    ${MARIADB_INCLUDE_DIRS}
    ${HIREDIS_INCLUDE_DIRS}
    include/network/protocol/generated
)

# 链接 AnimeCore 静态库及其他依赖
target_link_libraries(AnimeGameServerCore
    PUBLIC
    AnimeCore # <--- 链接公用底层库
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    Boost::system
    nlohmann_json::nlohmann_json
    ${Protobuf_LIBRARIES}
    ${MARIADB_LIBRARIES}
    ${HIREDIS_LIBRARIES}
)

# --- 6. 构建主程序 ---
add_executable(GameServer src/main.cpp)
target_link_libraries(GameServer
    PRIVATE
    AnimeGameServerCore
    nlohmann_json::nlohmann_json
    ${PROTOBUF_LIBRARIES}
    ${MARIADB_LIBRARIES}
    ${HIREDIS_LIBRARIES}
)

# --- 7. 平台相关配置 ---
if(WIN32)
    target_link_libraries(GameServer PRIVATE ws2_32 mswsock)
    target_compile_definitions(GameServer PRIVATE _WIN32_WINNT=0x0601)
    target_compile_definitions(AnimeGameServerCore PUBLIC HIREDIS_WIN)
endif()

# 自动拷贝配置文件到 build 目录
configure_file(
    "${CMAKE_SOURCE_DIR}/config/server.yaml"
    "${CMAKE_BINARY_DIR}/server.yaml"
    COPYONLY
)


# --- 8. 资源拷贝 ---
add_custom_command(TARGET AnimeGameServerCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/config ${CMAKE_CURRENT_BINARY_DIR}/config
)