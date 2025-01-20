#include "m7/m7_ipc_task.hh"

namespace coralmicro {

void m7_ipc_task(void* parameters) {
    (void)parameters;
    
    printf("M7 IPC task starting...\r\n");
    
    // Setup IPC
    
    // Initialize IPC
    auto* ipc = IpcM7::GetSingleton();
    // Register message handler
    ipc->RegisterAppMessageHandler(rx_data);
    // Start M4 core and wait for it to be ready
    ipc->StartM4();

    if (!ipc->M4IsAlive(500)) {
        printf("M7 IPC: Failed to start M4 core\r\n");
        update_state_event_m7(TaskID::M7_IPC_TASK, SystemEvents::BOOT_FAIL); 
        return;
    }
    printf("M7 IPC: M4 core started successfully\r\n");

    
    // Main task loop
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void rx_data(const uint8_t data[kIpcMessageBufferDataSize]) {
    const auto* msg = reinterpret_cast<const AppMessage*>(data);
    switch (msg->type) {
        case AppMessageType::kCameraData: {
            const auto* camera_data = reinterpret_cast<const CameraData*>(msg->data);

            // Place in inference input queue
            xQueueOverwrite(*M7IpcTaskQueues::inference_input_queue, camera_data);

            break;
        }
        case AppMessageType::kStateEvent: {
            const auto* state_event = reinterpret_cast<const StateEvent*>(msg->data);

            // Place in state event queue
            xQueueSendToBack(*M7IpcTaskQueues::state_event_queue, state_event, 0);

            break;
        }
        default:
            printf("M7 IPC: Unknown message type received\r\n");
            break;
    }
}

} // namespace coralmicro