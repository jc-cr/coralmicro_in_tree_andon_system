// depth_estimation_task.hh

#pragma once


#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"


#include <algorithm>
#include <unordered_map>

#include "runtime_config.hh"
#include "m7/m7_queues.hh"
#include "tof_rgb_mapping.hh"


namespace coralmicro {
    struct DepthCell {
        uint16_t distance_mm;
        uint8_t status;
        bool valid;
    };

    void depth_estimation_task(void* parameters);

    bool is_pixel_in_bbox(uint32_t x, uint32_t y, const tensorflow::BBox& bbox, uint32_t image_width, uint32_t image_height);

    std::vector<DepthCell> get_overlapping_depth_cells(const tensorflow::BBox& bbox, const VL53L8CX_ResultsData& tof_data, uint32_t image_width, uint32_t image_height);

    uint16_t get_depth_estimate(const std::unordered_map<uint16_t, uint32_t>& histogram);



}