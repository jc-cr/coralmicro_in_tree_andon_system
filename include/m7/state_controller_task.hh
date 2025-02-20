// state_controller_task.hh

#pragma once

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"


#include "m7/m7_queues.hh"
#include "global_config.hh"
#include "system_states.hh"

namespace coralmicro {
    void state_controller_task(void* parameters);
}