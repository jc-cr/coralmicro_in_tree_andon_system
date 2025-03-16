// depth_estimation.hh
#pragma once
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "libs/tensorflow/detection.h"
#include <cmath>


namespace coralmicro {
    // Only passing in the necessary data
    void depth_estimation(
        const tensorflow::Object* detections, // array of detections
        const uint8_t detection_count,    // number of detections 
        const int16_t* distance_mm,   //  array from ToF
        const uint8_t tof_resolution,       // Current ToF resolution (4x4 or 8x8)
        float* depths_out
    );
} // namespace coralmicro