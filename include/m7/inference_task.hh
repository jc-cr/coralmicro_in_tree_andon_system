// inference_task.hh
#pragma once

#include <vector>
#include "libs/base/filesystem.h"
#include "libs/tensorflow/detection.h"
#include "libs/tensorflow/utils.h"
#include "libs/tpu/edgetpu_manager.h"
#include "libs/tpu/edgetpu_op.h"
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_error_reporter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_interpreter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "m7/m7_queues.hh"

namespace coralmicro {
    // Task Constants
    constexpr char kModelPath[] = "/models/tf2_ssd_mobilenet_v2_coco17_ptq_edgetpu.tflite";
    constexpr int kTensorArenaSize = 8 * 1024 * 1024;
    STATIC_TENSOR_ARENA_IN_SDRAM(tensor_arena, kTensorArenaSize);

    // Queue Configuration
    struct InferenceTaskQueues {
        static constexpr QueueHandle_t* input_queue = &g_ipc_camera_queue_m7;    // Input camera data
        static constexpr QueueHandle_t* output_queue = &g_inference_queue_m7;    // Output inference results
    };

    // Task Functions
    void inference_task(void* parameters);
    bool detect_objects(tflite::MicroInterpreter* interpreter, 
                       const CameraData& camera_data,
                       std::vector<tensorflow::Object>* results);
}