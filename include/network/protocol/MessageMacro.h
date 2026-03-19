#pragma once

#include "network/protocol/MessageRegistry.h"

// 注册 protobuf 消息
#define REGISTER_PROTO_MESSAGE(MSG_ID, PB_TYPE) \
    MessageRegistry::RegisterProto<PB_TYPE>(MSG_ID)