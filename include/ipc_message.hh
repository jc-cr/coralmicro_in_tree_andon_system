// ipc_message.hh
#pragma once

#include "libs/base/ipc_message_buffer.h"

namespace coralmicro {

    enum class AppMessageType : uint8_t {
        kCameraData,
        kStateEvent,
    };

    struct AppMessage {
        AppMessageType type;
        uint8_t data[kIpcMessageBufferDataSize - sizeof(AppMessageType)];
    } __attribute__((packed));

    static_assert(sizeof(AppMessage) <= kIpcMessageBufferDataSize);
}