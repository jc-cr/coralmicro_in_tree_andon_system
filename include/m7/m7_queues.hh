#pragma once

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/queue.h"
#include "libs/camera/camera.h"
#include <vector>
#include <memory>


#include "libs/tensorflow/utils.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_error_reporter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_interpreter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "third_party/tflite-micro/tensorflow/lite/schema/schema_generated.h"

#include "libs/tensorflow/detection.h"


extern "C" {
#include "vl53l8cx_api.h"
}

namespace coralmicro {


// Structure for camera data
struct CameraData {
    uint32_t width;
    uint32_t height;
    CameraFormat format;
    TickType_t timestamp;
    std::shared_ptr<std::vector<uint8_t>> image_data;

    CameraData() : image_data(std::make_shared<std::vector<uint8_t>>()) {}
};


struct InferenceData {
    uint32_t model_width;
    uint32_t model_height;
    std::vector<tensorflow::Object> results;
    CameraData camera_data;
    TickType_t timestamp;

    InferenceData() : camera_data() {}
};


// Queue handles
inline QueueHandle_t g_tof_queue_m7;      // Latest TOF frame
inline QueueHandle_t g_ipc_camera_queue_m7;  // Latest camera frame from IPC transfer
inline QueueHandle_t g_inference_queue_m7;   // Latest inference results


// Queue creation
inline bool InitQueues() {
    g_tof_queue_m7 = xQueueCreate(1, sizeof(VL53L8CX_ResultsData));
    g_ipc_camera_queue_m7 = xQueueCreate(1, sizeof(CameraData));
    g_inference_queue_m7 = xQueueCreate(1, sizeof(InferenceData));
    
    return (g_tof_queue_m7 != nullptr && g_ipc_camera_queue_m7 != nullptr && g_inference_queue_m7 != nullptr);
}

// Queue cleanup
inline void CleanupQueues() {
    if (g_tof_queue_m7) vQueueDelete(g_tof_queue_m7);
    if (g_ipc_camera_queue_m7) vQueueDelete(g_ipc_camera_queue_m7);
    if (g_inference_queue_m7) vQueueDelete(g_inference_queue_m7);
}

}