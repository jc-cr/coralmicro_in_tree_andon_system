#include "m4/m4_ipc_task.hh"

namespace coralmicro {

    void m4_ipc_task(void* parameters) {
        (void)parameters;
        
        printf("M4 IPC task starting...\r\n");

        // Init data structures
        CameraData latest_camera_data;
        
        while (true) {
            tx_data(latest_camera_data);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    void tx_data(CameraData camera_data) {
        // Send pending IPC messages
        if (xQueuePeek(M4IpcTaskQueues::camera_queue(), &camera_data, 0) == pdTRUE) {
            IpcMessage camera_data_msg{};
            camera_data_msg.type = IpcMessageType::kApp;
            auto* app_msg = reinterpret_cast<AppMessage*>(&camera_data_msg.message.data);
            app_msg->type = AppMessageType::kCameraData;
            memcpy(app_msg->data, &camera_data, sizeof(CameraData));

            IpcM4::GetSingleton()->SendMessage(camera_data_msg);
        }
    }
} // namespace coralmicro