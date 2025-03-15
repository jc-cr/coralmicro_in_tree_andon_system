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
        VL53L8CX_ResultsData results_data = {};


        SystemState new_state = current_state;
        
        while (true) {
            // Default next state is current state
            new_state = current_state;
            
            // Try to get latest detection data
            if (xQueueReceive(g_detection_output_queue_m7, &detection_data, 0) == pdTRUE) {
                // Check that the pointer is valid before trying to access it
                if (detection_data.detections && detection_data.detections->size() > 0) {
                    printf("Person detected!\r\n");
                    new_state = SystemState::WARNING;
                } else {
                    new_state = SystemState::HOST_ACTIVE_STATE;
                }
            }
            
            // Check distance if in warning state
            if (current_state == SystemState::WARNING) {

                // Call TOF queue

                // Ensure detection data is valid before proceeding
                if (detection_data.detections && !detection_data.detections->empty()) {
                    printf("Checking distance...\r\n");
                    // Now call the depth estimation function with valid data
                    depth_estimation(detection_data, depth_estimation_data);
                    
                    // Process depth information (add your logic here)
                    if (!depth_estimation_data.depths.empty()) {

                    }
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