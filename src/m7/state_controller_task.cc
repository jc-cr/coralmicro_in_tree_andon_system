#include "m7/state_controller_task.hh"

namespace coralmicro{

    void state_controller_task(void* parameters) {
        (void)parameters;
        printf("State controller task starting...\r\n");
        
        SystemState current_state = SystemState::UNINITIALIZED;
        
        // Update to idle if uninitialized
        if (current_state == SystemState::UNINITIALIZED) {
            current_state = SystemState::HOST_READING;
            xQueueOverwrite(g_state_update_queue_m7, &current_state);
        }
        
        // Setup detection data
        DetectionData detection_data;
        DepthEstimationData depth_estimation_data;

        static VL53L8CX_ResultsData tof_data;

        SystemState new_state = current_state;
        
        while (true) {
            // Default next state is current state
            new_state = current_state;
            
            if (xQueueReceive(g_tof_queue_m7, &tof_data, 0) == pdTRUE) {
                // Print
                printf("TOF data received\r\n");
            }
            
            // Update system state if it has changed
            if (new_state != current_state) {
                xQueueOverwrite(g_state_update_queue_m7, &new_state);
                printf("System State: %i\r\n", static_cast<int>(new_state));
                current_state = new_state;
            }
            
            // Task delay
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}