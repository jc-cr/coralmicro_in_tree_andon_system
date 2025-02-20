// m7_queues.hh
#pragma once

#include <vector>
#include <memory>

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/queue.h"

// Camera
#include "libs/camera/camera.h"

// TOF
extern "C" {
#include "vl53l8cx_api.h"
}


#include "libs/base/filesystem.h"
#include "libs/tensorflow/detection.h"
#include "libs/tensorflow/utils.h"

#include "system_states.hh"

namespace coralmicro {

    struct CameraData {
        uint32_t width;
        uint32_t height;
        CameraFormat format;
        TickType_t timestamp;
        std::shared_ptr<std::vector<uint8_t>> image_data;

        CameraData() : image_data(std::make_shared<std::vector<uint8_t>>()) {}
    };

    struct DetectionData {
        CameraData camera_data;                           // Original camera data
        std::vector<tensorflow::Object> detections;       // Detection results
        TickType_t timestamp;                            // Timestamp of the detection
        TickType_t inference_time;                        // Time taken for inference
    };

    // Queue handles
    inline QueueHandle_t g_tof_queue_m7;      // Latest TOF frame
    inline QueueHandle_t g_camera_queue_m7;   // Latest camera frame
    inline QueueHandle_t g_detection_output_queue_m7; // Detection results

    // Queue creation
    inline bool InitQueues() {
        g_tof_queue_m7 = xQueueCreate(1, sizeof(VL53L8CX_ResultsData));
        g_camera_queue_m7 = xQueueCreate(1, sizeof(CameraData));
        g_detection_output_queue_m7 = xQueueCreate(1, sizeof(DetectionData));
        
        return (g_tof_queue_m7 != nullptr && g_camera_queue_m7 != nullptr);
    }

    // Queue cleanup
    inline void CleanupQueues() {
        if (g_tof_queue_m7) vQueueDelete(g_tof_queue_m7);
        if (g_camera_queue_m7) vQueueDelete(g_camera_queue_m7);
        if (g_detection_output_queue_m7) vQueueDelete(g_detection_output_queue_m7);
    }

}