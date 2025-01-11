#include "m4/m4_ipc_task.hh"

namespace coralmicro {


    void m4_ipc_task(void* parameters) {
        (void)parameters;
        
        printf("M4 IPC task starting...\r\n");
        
        // Register IPC message handler
        IpcM4::GetSingleton()->RegisterAppMessageHandler(
            [](const uint8_t data[kIpcMessageBufferDataSize]) {
                // Handle incoming messages from M7 if needed
            });
        
        
        while (true) {
            process_sensor_data_m4();  // Check for new sensor data
            tx_data();        // Send pending IPC messages

            vTaskDelay(pdMS_TO_TICKS(10));

        }
    }

    void process_sensor_data_m4() {
        // Process Camera Data
        CameraData camera_data;
        if (xQueuePeek(g_camera_queue_m4, &camera_data, 0) == pdTRUE) {
            g_m4_ipc_manager.sendMessage(
                SensorType::kCamera,
                camera_data.image_data->data(),
                camera_data.image_data->size()
            );
        }
        
    }

    void tx_data() {
        static uint8_t buffer[4096];  // Temp buffer for message assembly
        size_t size;
        
        // Try to send camera data
        if (g_m4_ipc_manager.receiveMessage(SensorType::kCamera, buffer, size)) {
            IpcMessage msg{};
            msg.type = IpcMessageType::kApp;
            memcpy(msg.message.data, buffer, std::min(size, sizeof(msg.message.data)));
            IpcM4::GetSingleton()->SendMessage(msg);
        }
        
    }

} // namespace coralmicro