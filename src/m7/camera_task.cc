// camera_task.cc
#include "m7/camera_task.hh"

namespace coralmicro {

bool first_frame_captured_flag = false;

void camera_task(void* parameters) {
    (void)parameters;
    printf("Camera task starting...\r\n");

    // Initialize camera with proper error checking
    if (!CameraTask::GetSingleton()->SetPower(true)) {
        printf("Failed to power on camera\r\n");
        vTaskSuspend(nullptr);
        return;
    }

    if (!CameraTask::GetSingleton()->Enable(CameraMode::kStreaming)) {
        printf("Failed to enable camera streaming\r\n");
        vTaskSuspend(nullptr);
        return;
    }

    // Discard initial frames to allow auto-exposure calibration
    CameraTask::GetSingleton()->DiscardFrames(100);

    // Create two alternating buffers to ensure data consistency
    std::shared_ptr<std::vector<uint8_t>> buffer1 = std::make_shared<std::vector<uint8_t>>();
    std::shared_ptr<std::vector<uint8_t>> buffer2 = std::make_shared<std::vector<uint8_t>>();
    
    const size_t buffer_size = CameraConfig::kWidth * CameraConfig::kHeight * 
                              CameraFormatBpp(CameraConfig::kFormat);
    
    buffer1->resize(buffer_size);
    buffer2->resize(buffer_size);
    
    std::shared_ptr<std::vector<uint8_t>> current_buffer = buffer1;
    
    CameraData camera_data;
    camera_data.width = CameraConfig::kWidth;
    camera_data.height = CameraConfig::kHeight;
    camera_data.format = CameraConfig::kFormat;

    // Track timing for consistent frame rate
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t capture_period = pdMS_TO_TICKS(10);

    while (true) {
        // Setup frame format with current buffer
        CameraFrameFormat fmt{
            camera_data.format,
            CameraConfig::filter,
            CameraConfig::rotation,
            static_cast<int>(camera_data.width),
            static_cast<int>(camera_data.height),
            CameraConfig::preserve_ratio,
            current_buffer->data(),
            CameraConfig::auto_white_balance
        };

        if (CameraTask::GetSingleton()->GetFrame({fmt})) {
            camera_data.timestamp = xTaskGetTickCount();
            camera_data.image_data = current_buffer;  // Assign current buffer
            
            // Send to queue
            if (xQueueOverwrite(g_camera_queue_m7, &camera_data) == pdTRUE) {
                // Switch buffers only after successful queue write
                current_buffer = (current_buffer == buffer1) ? buffer2 : buffer1;
            }
        }
        
        // Use vTaskDelayUntil for consistent frame timing
        vTaskDelayUntil(&last_wake_time, capture_period);
    }
}


} // namespace coralmicro