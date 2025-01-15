// depth_estimation_task.hh

#pragma once


#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"


#include "runtime_config.hh"
#include "m7/m7_queues.hh"
#include "tof_rgb_mapping.hh"


namespace coralmicro {



    // Task for depth estimation
    // Input: Inference data, tof rgb mapping, TOF depth data
    void depth_estimation_task(void* parameters);



}