cmake_minimum_required(VERSION 3.16)
project(AnimeGameServer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# --- 1. 查找依赖 ---
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(GTest REQUIRED)
find_package(Protobuf REQUIRED)
find_package(absl REQUIRED CONFIG)
find_package(PkgConfig REQUIRED)
find_package(nlohmann_json REQUIRED)

# 使用 PkgConfig 查找 Protobuf, MySQL (MariaDB) 和 Redis (hiredis)
pkg_check_modules(PROTOBUF REQUIRED protobuf)
pkg_check_modules(MARIADB REQUIRED libmariadb)
pkg_check_modules(HIREDIS REQUIRED hiredis) # <--- 新增：查找 hiredis 库

# --- 2. Protobuf 配置 ---
set(PROTO_DIR "include/network/protocol/proto")
set(PROTO_FILES 
    "${PROTO_DIR}/login.proto"
    "${PROTO_DIR}/gacha.proto"
    "${PROTO_DIR}/heartbeat.proto"
)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# --- 3. 包含目录 ---
include_directories(include)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${MARIADB_INCLUDE_DIRS})
include_directories(${HIREDIS_INCLUDE_DIRS}) # <--- 新增：包含 Redis 头文件路径

# --- 4. 源文件管理 ---
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

# --- 5. 构建核心库 ---
add_library(AnimeGameServerCore ${SRC_FILES} ${PROTO_SRCS} ${PROTO_HDRS})

target_include_directories(AnimeGameServerCore PUBLIC 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
    ${MARIADB_INCLUDE_DIRS}
    ${HIREDIS_INCLUDE_DIRS} # <--- 新增
)

target_link_libraries(AnimeGameServerCore
    PUBLIC
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    Boost::system
    nlohmann_json::nlohmann_json
    ${Protobuf_LIBRARIES}
    ${MARIADB_LIBRARIES}
    ${HIREDIS_LIBRARIES}    # <--- 新增：链接 hiredis 库
)

# --- 6. 构建主程序 ---
add_executable(GameServer src/main.cpp)
target_link_libraries(GameServer 
    PRIVATE 
    AnimeGameServerCore
    nlohmann_json::nlohmann_json
    ${PROTOBUF_LIBRARIES}
    ${MARIADB_LIBRARIES}
    ${HIREDIS_LIBRARIES}    # <--- 新增
)

# --- 8. 平台相关配置 ---
if(WIN32)
    target_link_libraries(GameServer PRIVATE ws2_32 mswsock)
    target_compile_definitions(GameServer PRIVATE _WIN32_WINNT=0x0601)
    
    # 针对 Windows 下 hiredis 的特殊宏定义，防止符号导出/兼容性问题
    target_compile_definitions(AnimeGameServerCore PUBLIC HIREDIS_WIN)
endif()

# --- 9. 资源拷贝 ---
add_custom_command(TARGET AnimeGameServerCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/config ${CMAKE_CURRENT_BINARY_DIR}/config
)