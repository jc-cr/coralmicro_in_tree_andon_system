// m7_ipc_task.hh
#pragma once

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"


#include "m7/m7_queues.hh"
#include "libs/base/ipc_m7.h"

namespace coralmicro{
    void m7_ipc_task(void* parameters);

    void process_sensor_data_m7();
    void tx_data();
    void rx_data();
}