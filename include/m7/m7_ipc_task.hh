// m7_ipc_task.hh
#pragma once

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"


#include "m7/m7_queues.hh"
#include "libs/base/ipc_m7.h"
#include "system_state.hh"

namespace coralmicro{
    void m7_ipc_task(void* parameters);
    void rx_data();

    struct M7IpcTaskQueues{
        static constexpr QueueHandle_t* inference_input_queue = &g_inference_input_queue_m7;
        static constexpr QueueHandle_t* state_event_queue = &g_state_event_queue_m7;
    };
}