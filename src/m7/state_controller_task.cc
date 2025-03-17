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
        static LoggingData logging_data;

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
                if (xQueueReceive(g_detection_output_queue_m7, &detection_data, 10) == pdTRUE) {

                    if (detection_data.detection_count > 0)
                    {
                        // Person detected, change state to WARNING
                        new_state = SystemState::WARNING;
                    } 
                } 
            }

            // If in WARNING state, check if person in danger distance
            if (new_state == SystemState::WARNING){
                printf("Checking for danger distance\r\n");
                // Get the latest TOF data
                if (xQueueReceive(g_tof_queue_m7, &tof_data, 10) == pdTRUE) {

                    // Set depth estimation (DE) timestamp
                    depth_estimation_data.timestamp = xTaskGetTickCount();

                    // Perform depth estimation
                    depth_estimation(
                        detection_data.detections, 
                        detection_data.detection_count, 
                        tof_data.distance_mm, 
                        depth_estimation_data.depths
                    );

                    // Update depth estimation time
                    depth_estimation_data.depth_estimation_time = xTaskGetTickCount() - depth_estimation_data.timestamp;

                    printf("Depths for detections: ");
                    for (uint8_t i = 0; i < detection_data.detection_count; i++) {
                        printf("%f ", depth_estimation_data.depths[i]);
                    }   
                    printf("\r\n");


                    // Check if any depth is below the danger distance threshold
                    for (uint8_t i = 0; i < detection_data.detection_count; i++) {
                        if (depth_estimation_data.depths[i] <= g_danger_depth.load()) {

                            new_state = SystemState::STOPPED;

                            // Set system fault
                            SystemFault fault = SystemFault::ESTOPPED;
                        }
                    }
                }
            }

            // Update system state if it has changed
            if (new_state != current_state) {
                xQueueOverwrite(g_state_update_queue_m7, &new_state);
                printf("System State: %i\r\n", static_cast<int>(new_state));
                current_state = new_state;
            }

            // Output logging data structure to queue
            logging_data.timestamp = xTaskGetTickCount();
            logging_data.system_state = current_state;
            logging_data.detection_data = detection_data;
            logging_data.depth_estimation_data = depth_estimation_data;
            if (xQueueOverwrite(g_logging_queue_m7, &logging_data) != pdTRUE) {
                printf("ERROR: Failed to send logging data\r\n");
            }

            // Task delay
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}