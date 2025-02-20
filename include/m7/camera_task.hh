// camera_task.hh

#pragma once

#include "libs/camera/camera.h"
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"

#include "m7/m7_queues.hh"


namespace coralmicro{

    struct CameraConfig{
        static constexpr CameraFormat kFormat = CameraFormat::kRgb;
        static constexpr CameraFilterMethod filter = CameraFilterMethod::kBilinear;
        static constexpr CameraRotation rotation = CameraRotation::k270;
        static constexpr uint32_t kWidth = 300;
        static constexpr uint32_t kHeight = 300;
        static constexpr bool preserve_ratio = true;
        static constexpr bool auto_white_balance = false;
    };

    void camera_task(void* parameters);
}