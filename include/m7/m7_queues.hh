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
        CameraData camera_data;
        std::shared_ptr<std::vector<tensorflow::Object>> detections;
        TickType_t timestamp;

        TickType_t inference_time;
        
        DetectionData() : detections(std::make_shared<std::vector<tensorflow::Object>>()) {}
    };

    struct DetectionDepth {
        tensorflow::Object detection;
        float depth_mm;
        bool valid;
    };

    struct DepthEstimationData {
        std::shared_ptr<std::vector<DetectionDepth>> detection_depths;  // Changed to shared_ptr
        CameraData camera_data;
        TickType_t timestamp;
        TickType_t detection_data_inference_time;
        TickType_t depth_estimation_time;
        
        DepthEstimationData() : detection_depths(std::make_shared<std::vector<DetectionDepth>>()) {}
    };

    struct LoggingData {
        SystemState state;
        DepthEstimationData depth_estimation_data;
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

        g_host_connection_status_queue_m7 = xQueueCreate(1, sizeof(HostConnectionStatus));
        g_host_state_queue_m7 = xQueueCreate(1, sizeof(HostState));



        g_logging_queue_m7 = xQueueCreate(1, sizeof(LoggingData));
        
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