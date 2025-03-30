// state_controller_task.hh

#pragma once

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"


#include "m7/m7_queues.hh"
#include "global_config.hh"
#include "system_enums.hh"
#include "depth_estimation.hh"

namespace coralmicro {

    void state_controller_task(void* parameters);

    // Timeout limit in ticks - 3 seconds (assuming 1ms tick rate)
    constexpr TickType_t kValidConnectionLimitTicks = pdMS_TO_TICKS(3000); 

    constexpr float danger_depth_mm = 300.0f; // Danger distance in mm
}