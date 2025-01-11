#include "m7/m7_ipc_task.hh"

namespace coralmicro {

void m7_ipc_task(void* parameters) {
    (void)parameters;
    
    printf("M7 IPC task starting...\r\n");
    
    // Initialize IPC
    auto* ipc = IpcM7::GetSingleton();
    
    // Register message handler
    ipc->RegisterAppMessageHandler(
        [](const uint8_t data[kIpcMessageBufferDataSize]) {
            // Create camera data structure
            CameraData camera_data;
            
            // Get the data size from the first 4 bytes
            uint32_t data_size;
            memcpy(&data_size, data, sizeof(uint32_t));
            
            // Set basic camera parameters (these should match M4 settings)
            camera_data.width = 324;
            camera_data.height = 324;
            camera_data.format = CameraFormat::kRgb;
            camera_data.timestamp = xTaskGetTickCount();
            
            // Copy image data
            camera_data.image_data->resize(data_size);
            memcpy(camera_data.image_data->data(), 
                   data + sizeof(uint32_t), 
                   data_size);
            
            // Send to camera queue for other tasks to use
            xQueueOverwrite(g_ipc_camera_queue_m7, &camera_data);
        });
    
    // Start M4 core and wait for it to be ready
    ipc->StartM4();
    if (!ipc->M4IsAlive(500)) {
        printf("M7 IPC: Failed to start M4 core\r\n");
        vTaskSuspend(nullptr);
        return;
    }
    
    printf("M7 IPC: M4 core started successfully\r\n");
    
    // Main task loop
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10));  // Match M4's timing
    }
}

} // namespace coralmicro