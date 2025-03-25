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

#include "system_enums.hh"
#include "global_config.hh"

namespace coralmicro {

    struct CameraData {
        TickType_t timestamp_ms; // timestamp_ms of creation (ms)

        uint32_t width; 
        uint32_t height;
        CameraFormat format; // kRGB, kYUV, etc.

        std::shared_ptr<std::vector<uint8_t>> image_data; // Shared pointer to image data buffer
        CameraData() : image_data(std::make_shared<std::vector<uint8_t>>()) {}
    };

    struct DetectionData {
        TickType_t timestamp_ms;   // timestamp_ms of creation (ms)

        tensorflow::Object detections[g_max_detections_per_inference]; // Array to hold detection results
        uint8_t detection_count; // Actual number of valid detections
        
        TickType_t inference_time_ms; // time taken for inference

        CameraData camera_data; 
        
        DetectionData() : detection_count(0) {}
    };


    struct DepthEstimationData {
        TickType_t timestamp_ms; // timestamp_ms of creation (ms)

        float depths[g_max_detections_per_inference]; // Array to hold estimated depths for each detection

        TickType_t depth_estimation_time_ms; // Time taken for depth estimation (ms)
    };

    struct LoggingData {
        TickType_t timestamp_ms; // timestamp_ms of creation

        SystemState system_state; // Current system state

        DetectionData detection_data; // Detection data
        DepthEstimationData depth_estimation_data; // Depth estimation data
    };


    // Queue handles
    inline QueueHandle_t g_tof_queue_m7;      // Latest TOF frame
    inline QueueHandle_t g_camera_queue_m7;   // Latest camera frame
    inline QueueHandle_t g_detection_output_queue_m7; // Detection results

    inline QueueHandle_t g_state_update_queue_m7; // State updates
    inline QueueHandle_t g_host_connection_status_queue_m7; // Host condition updates
    inline QueueHandle_t g_host_state_queue_m7; // Host state updates

    inline QueueHandle_t g_logging_queue_m7; // Logging data


    // Queue creation
    inline bool InitQueues() {
        g_tof_queue_m7 = xQueueCreate(1, sizeof(VL53L8CX_ResultsData));
        g_camera_queue_m7 = xQueueCreate(1, sizeof(CameraData));

        g_detection_output_queue_m7 = xQueueCreate(1, sizeof(DetectionData));

        g_state_update_queue_m7 = xQueueCreate(1, sizeof(SystemState));

        g_logging_queue_m7 = xQueueCreate(1, sizeof(LoggingData));


        g_host_connection_status_queue_m7 = xQueueCreate(1, sizeof(HostConnectionStatus));
        g_host_state_queue_m7 = xQueueCreate(1, sizeof(HostState));

        
        return (g_tof_queue_m7 != nullptr && g_camera_queue_m7 != nullptr);
    }

    // Queue cleanup
    inline void CleanupQueues() {
        if (g_tof_queue_m7) vQueueDelete(g_tof_queue_m7);
        if (g_camera_queue_m7) vQueueDelete(g_camera_queue_m7);
        if (g_detection_output_queue_m7) vQueueDelete(g_detection_output_queue_m7);

        if (g_state_update_queue_m7) vQueueDelete(g_state_update_queue_m7);
        if (g_host_connection_status_queue_m7) vQueueDelete(g_host_connection_status_queue_m7);
        if (g_host_state_queue_m7) vQueueDelete(g_host_state_queue_m7);

        if (g_logging_queue_m7) vQueueDelete(g_logging_queue_m7);
    }
}