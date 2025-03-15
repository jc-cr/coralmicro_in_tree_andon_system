// depth_estimation_task.hh
#pragma once

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"

#include "tof_rgb_mapping.hh"

#include "m7/m7_queues.hh"
#include "global_config.hh"

#include <algorithm>
#include <cmath>


namespace coralmicro {

    void depth_estimation(const DetectionData& detection_data, 
        const VL53L8CX_ResultsData& tof_data,
        DepthEstimationData& output_data);

} // namespace coralmicro

