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

        update_state_event_m4(TaskID::M4_IPC_TASK, SystemEvents::BOOT_SUCCESS);
        
        while (true) {
            tx_data();        // Send pending IPC messages

            vTaskDelay(pdMS_TO_TICKS(10));

        }
    }

    void tx_data() {
        // Send pending IPC messages
        if (xQueuePeek(M4IpcTaskQueues::camera_queue, &camera_data, 0) == pdTRUE) {
            IpcMessage camera_data_msg{};
            camera_data_msg.type = IpcMessageType::kApp;
            auto* app_msg = reinterpret_cast<AppMessage*>(&camera_data_msg.message.data);
            app_msg->type = AppMessageType::kCameraData;
            memcpy(app_msg->data, &camera_data, sizeof(CameraData));

            IpcM4::GetSingleton()->SendMessage(camera_data_msg);
        }

        // Using xQueueReceive as state event queue is multimessage queue
        if (xQueueReceive(M4IpcTaskQueues::state_event_queue, &state_event_msg, 0) == pdPASS) {
            IpcMessage state_event_msg{};
            state_event_msg.type = IpcMessageType::kApp;
            auto* app_msg = reinterpret_cast<AppMessage*>(&state_event_msg.message.data);
            app_msg->type = AppMessageType::kStateEvent;
            memcpy(app_msg->data, &state_event_msg, sizeof(StateEventUpdateMessage));

            IpcM4::GetSingleton()->SendMessage(state_event_msg);
        }

    }
} // namespace coralmicro