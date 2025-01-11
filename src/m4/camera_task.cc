// camera_task.cc
#include "m4/camera_task.hh"

namespace coralmicro {

    void camera_task(void* parameters) {
        (void)parameters;

        printf("Camera task starting...\r\n");

        // Initialize camera
        CameraTask::GetSingleton()->SetPower(true);
        CameraTask::GetSingleton()->Enable(CameraMode::kStreaming);

        // Main task loop
        while (true) {
            // Create new camera data instance
            CameraData camera_data;
            camera_data.width = 324;
            camera_data.height = 324;
            camera_data.format = CameraFormat::kRgb;
            camera_data.timestamp = xTaskGetTickCount();
            
            // Use vector directly like in the example
            camera_data.image_data->resize(
                camera_data.width * camera_data.height * 
                CameraFormatBpp(camera_data.format));

            CameraFrameFormat fmt{
                camera_data.format,
                CameraFilterMethod::kBilinear,
                CameraRotation::k270,
                static_cast<int>(camera_data.width),
                static_cast<int>(camera_data.height),
                false,
                camera_data.image_data->data(),
                false  // auto white balance off
            };

            if (CameraTask::GetSingleton()->GetFrame({fmt})) {
                // Now safe to send to queue
                if (xQueueOverwrite(g_camera_queue_m4, &camera_data) != pdTRUE) {
                    printf("Failed to send camera data to queue\r\n");
                }
            } else {
                printf("Camera frame capture failed\r\n");
            }

            vTaskDelay(pdMS_TO_TICKS(33));  // ~30 FPS
        }
    }
}