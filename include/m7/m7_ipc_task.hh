// m7_ipc_task.hh
#pragma once

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"

#include "state_definitions.hh"
#include "m7/m7_queues.hh"
#include "libs/base/ipc_m7.h"
#include "ipc_message.hh"

namespace coralmicro{
    void m7_ipc_task(void* parameters);
    void rx_data(const uint8_t data[kIpcMessageBufferDataSize]);

    struct M7IpcTaskQueues{
        static constexpr QueueHandle_t* inference_input_queue = &g_inference_input_queue_m7;
    };
}