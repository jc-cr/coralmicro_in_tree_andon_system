#pragma once

#include <vector>
#include <memory>

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/queue.h"

// Camera
#include "libs/camera/camera.h"

//.Inference
#include "libs/tensorflow/utils.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_error_reporter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_interpreter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "third_party/tflite-micro/tensorflow/lite/schema/schema_generated.h"
#include "libs/tensorflow/detection.h"


// TOF
extern "C" {
#include "vl53l8cx_api.h"
}

#include "state_definitions.hh"

namespace coralmicro {

    // Data structures

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

    struct TofData {
        VL53L8CX_ResultsData tof_results;
        TickType_t timestamp;
    };

    struct DepthEstimationData {
        InferenceData inference_data;
        TofData tof_data;
        // TODO: add person distance estimation
    };
    


    // Queue handles
    inline QueueHandle_t g_inference_input_queue_m7;  // Latest camera frame from IPC transfer

    inline QueueHandle_t g_state_event_queue_m7;  // Latest state events from m7 and m4 ipc

    inline QueueHandle_t g_tof_queue_m7;      // Latest TOF frame
    inline QueueHandle_t g_inference_output_queue_m7;   // Latest inference results


    // Queue creation
    inline bool InitQueues() {

        g_inference_input_queue_m7 = xQueueCreate(1, sizeof(CameraData));
        g_state_event_queue_m7 = xQueueCreate(3, sizeof(StateEventUpdateMessage)); // Able to hold 3 state events to prevent overwriting of events

        g_tof_queue_m7 = xQueueCreate(1, sizeof(TofData));
        g_inference_output_queue_m7 = xQueueCreate(1, sizeof(InferenceData));

        
        return (g_tof_queue_m7 != nullptr 
            && g_inference_input_queue_m7 != nullptr 
            && g_inference_output_queue_m7 != nullptr 
            && g_state_event_queue_m7 != nullptr);
    }

    // Queue cleanup
    inline void CleanupQueues() {
        if (g_inference_input_queue_m7) vQueueDelete(g_inference_input_queue_m7);
        if (g_state_event_queue_m7) vQueueDelete(g_state_event_queue_m7);

        if (g_tof_queue_m7) vQueueDelete(g_tof_queue_m7);
        if (g_inference_output_queue_m7) vQueueDelete(g_inference_output_queue_m7);
    }

}