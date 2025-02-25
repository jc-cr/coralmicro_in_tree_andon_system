// state_controller_task.cc

#include "m7/state_controller_task.hh"

namespace coralmicro{

    void state_controller_task(void* parameters) {
        (void)parameters;
        printf("State controller task starting...\r\n");
        
        SystemState current_state = g_system_state.load();

        // Update to idle if uninitialized
        if (current_state == SystemState::UNINITIALIZED) {
            g_system_state.store(SystemState::IDLE);
            current_state = SystemState::IDLE;
        }

        // Print current state
        printf("System State: %i\r\n", static_cast<int>(current_state));


        SystemState new_state = current_state;

        DetectionData detection_data;

        while (true) {
            // Determine new state based on conditions
            /*
            From IDLE of WARNING, get person detection
            If person detected, set state to WARNING
            If person not detected, set state to IDLE
            
            */

            if (xQueueReceive(g_detection_output_queue_m7, &detection_data, 0) == pdTRUE) {
                // Check that the pointer is valid before trying to access it
                if (detection_data.detections && detection_data.detections->size() > 0) {
                    new_state = SystemState::WARNING;
                } else {
                    new_state = SystemState::IDLE;
                }
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