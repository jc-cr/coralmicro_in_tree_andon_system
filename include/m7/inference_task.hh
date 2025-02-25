// inference_task.hh
#pragma once

#include <vector>


#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"

#include "libs/base/filesystem.h"
#include "libs/tensorflow/detection.h"
#include "libs/tensorflow/utils.h"

#include "libs/tpu/edgetpu_manager.h"
#include "libs/tpu/edgetpu_op.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_error_reporter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_interpreter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_mutable_op_resolver.h"

#include "m7/m7_queues.hh"
#include "global_config.hh"

namespace coralmicro {
    // Task Functions
    void inference_task(void* parameters);

    bool detect_objects(tflite::MicroInterpreter* interpreter, 
                       const CameraData& camera_data,
                       std::vector<tensorflow::Object>* results);

}