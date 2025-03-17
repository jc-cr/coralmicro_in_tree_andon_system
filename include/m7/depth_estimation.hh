// depth_estimation.hh
#pragma once
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "libs/tensorflow/detection.h"
#include <cmath>

#include "global_config.hh"
#include "tof_rgb_mapping.hh"


namespace coralmicro {

    void depth_estimation(
        const tensorflow::Object* detections, // array of detections
        const uint8_t detection_count,    // number of detections 
        const int16_t* distance_mm,   //  array from ToF
        float* depths_out
    );
} // namespace coralmicro