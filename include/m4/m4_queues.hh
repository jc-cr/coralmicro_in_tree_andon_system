// m4_queues.hh
#pragma once


#include <vector>
#include <memory>

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/queue.h"
#include "libs/camera/camera.h"


extern "C" {
#include "vl53l8cx_api.h"
}

namespace coralmicro {

    // Queue data structures
    struct CameraData {
        uint32_t width;
        uint32_t height;
        CameraFormat format;
        TickType_t timestamp;
        std::shared_ptr<std::vector<uint8_t>> image_data;

        CameraData() : image_data(std::make_shared<std::vector<uint8_t>>()) {}
    };

    // Queue handles
    inline QueueHandle_t g_camera_queue_m4;      // Latest camera frame
    inline QueueHandle_t g_tof_queue_m4;         // Latest TOF frame


    // Queue creation
    inline bool InitQueues() {
        g_camera_queue_m4 = xQueueCreate(1, sizeof(CameraData));
        g_tof_queue_m4 = xQueueCreate(1, sizeof(VL53L8CX_ResultsData));

        return (g_camera_queue_m4 != nullptr && g_tof_queue_m4 != nullptr);
        
    }

    // Queue cleanup
    inline void CleanupQueues() {
        if (g_camera_queue_m4) vQueueDelete(g_camera_queue_m4);
        if (g_tof_queue_m4) vQueueDelete(g_tof_queue_m4);
    }


}