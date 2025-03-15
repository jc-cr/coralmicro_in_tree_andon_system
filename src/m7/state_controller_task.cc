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
        static DetectionData detection_data;
        static DepthEstimationData depth_estimation_data;
        static VL53L8CX_ResultsData tof_data;

        SystemState new_state = current_state;
        
        while (true) {
            // Default next state is current state
            new_state = current_state;

            // DEBUG
            new_state = SystemState::SCANNING;
            
            // Check 
            if (new_state == SystemState::SCANNING){
            // Check if a person has been detected in the latest detection data
            // this done by checking if detection_data.detections is not empty
                if (xQueueReceive(g_detection_output_queue_m7, &detection_data, 3) == pdTRUE) {
                    if (detection_data.detections && !detection_data.detections->empty()) {
                        // Person detected, change state to WARNING
                        new_state = SystemState::WARNING;
                    } 
                } 
            }

            // If in WARNING state, check if person in danger distance
            if (new_state == SystemState::WARNING){
                printf("Checking for danger distance\r\n");
                // Get the latest TOF data
                if (xQueueReceive(g_tof_queue_m7, &tof_data, 3) == pdTRUE) {

                    // Perfrom depth estimation
                    depth_estimation(detection_data, tof_data, depth_estimation_data);

                    // TODO

                }
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