// state_controller_task.cc

#include "m7/state_controller_task.hh"

namespace coralmicro{

    void state_controller_task(void* parameters) {
        (void)parameters;
        printf("State controller task starting...\r\n");
        
        SystemState current_state = g_system_state.load();
        SystemState new_state;

        // Update to idle if uninitialized
        if (current_state == SystemState::UNINITIALIZED) {
            g_system_state.store(SystemState::IDLE);
            current_state = SystemState::IDLE;
        }

        // Print current state
        printf("System State: %i\r\n", current_state);
        
        while (true) {

            // Determine new state based on conditions
            // If person detected set to state to warning and move data to depth estimation queue
            // If no person detected set state to idle, update logging data struct and push to logging queue

            DetectionData detection_data;
            if (xQueuePeek(g_detection_output_queue_m7, &detection_data, 0) == pdTRUE) {
                if (detection_data.detections.size() > 0) {
                    new_state = SystemState::WARNING;
                } else {
                    new_state = SystemState::IDLE;
                }
            } 
            // Empty queue
            else {
                new_state = SystemState::IDLE;
            }
            
            
            if (new_state != current_state) {
                switch (new_state) {
                    case SystemState::IDLE:
                        g_system_state.store(SystemState::IDLE);
                        printf("System State: IDLE\r\n");
                        break;
                    case SystemState::WARNING:
                        g_system_state.store(SystemState::WARNING);
                        printf("System State: WARNING\r\n");
                        break;
                    case SystemState::ERROR:
                        g_system_state.store(SystemState::ERROR);
                        printf("System State: ERROR\r\n");
                        break;
                    default:
                        printf("System State: UNKNOWN\r\n");
                        break;
                }
                
                current_state = new_state;
            }
            
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}