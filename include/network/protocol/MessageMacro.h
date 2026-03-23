#pragma once

#include "network/protocol/MessageRegistry.h"

/**
 * @brief 注册 Protobuf 消息到全局注册表
 * @note 修复了 backslash-newline 警告，并确保在 namespace anime 下工作
 */
#define REGISTER_PROTO_MESSAGE(MSG_ID, PB_TYPE) \
    MessageRegistry::RegisterProto<PB_TYPE>(MSG_ID)

// 💡 关键：确保宏定义的最后一行是一个空行，
// 且上一行的末尾没有多余的反斜杠 \ 或空格。