// camera_task.hh

#pragma once

#include "libs/camera/camera.h"
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"

#include "m4/m4_queues.hh"

namespace coralmicro{

    struct CameraTaskQueues{
        static constexpr QueueHandle_t* output_queue = &g_camera_queue_m4;
    };

    struct CameraConfig{
        static constexpr uint32_t kWidth = 324;
        static constexpr uint32_t kHeight = 324;
        static constexpr CameraFormat kFormat = CameraFormat::kRgb;
        static constexpr CameraFilterMethod filter = CameraFilterMethod::kBilinear;
        static constexpr CameraRotation rotation = CameraRotation::k270;
        static constexpr bool auto_white_balance = false;
    };

    void camera_task(void* parameters);
}