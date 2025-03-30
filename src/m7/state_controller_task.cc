#include "m7/state_controller_task.hh"

namespace coralmicro{

    void state_logic_host_connected(HostState& host_state, SystemState& current_state, SystemState& new_state, DetectionData& detection_data, DepthEstimationData& depth_estimation_data, VL53L8CX_ResultsData& tof_data, LoggingData& logging_data) {

        // Check if host is in RED state
        if (RedStates::Contains(host_state)) {
            // If host is in RED state, set system state to STOPPED
            new_state = SystemState::STOPPED;
        } 
        else if (YellowStates::Contains(host_state)) {
            // If host is in YELLOW state, set system state to WARNING
            new_state = SystemState::WARNING;
        }
        else if (BlueStates::Contains(host_state)) {
            // If host is in BLUE state, set system state to IDLE
            new_state = SystemState::IDLE;
        } 
        else if (GreenStates::Contains(host_state)) {
            // If host is in GREEN state, set system state to SCANNING
            new_state = SystemState::ACTIVE;
        } 

        // Update system state if it has changed
        // THis is just so Andon light reflects the current state
        if (new_state != current_state) {
            xQueueOverwrite(g_state_update_queue_m7, &new_state);
            printf("System State: %i\r\n", static_cast<int>(new_state));
            current_state = new_state;
        }

        // Update to scanning state
        new_state = SystemState::SCANNING;

        if (new_state == SystemState::SCANNING){
        // Check if a person has been detected in the latest detection data
        // this done by checking if detection_data.detections is not empty
            xQueueReceive(g_detection_output_queue_m7, &detection_data, 10);
        }


        TickType_t depth_estimation_start_tick;
        TickType_t depth_estimation_stop_tick;

        if (detection_data.detection_count > 0)
        {
            printf("Checking for danger distance\r\n");
            // Get the latest TOF data
            if (xQueueReceive(g_tof_queue_m7, &tof_data, 10) == pdTRUE) {

                // Set depth estimation (DE) timestamp
                depth_estimation_start_tick = xTaskGetTickCount();

                depth_estimation_data.timestamp_ms = depth_estimation_start_tick * (1000 / configTICK_RATE_HZ);

                // Perform depth estimation
                depth_estimation(
                    detection_data.detections, 
                    detection_data.detection_count, 
                    tof_data.distance_mm, 
                    depth_estimation_data.depths
                );

                // Update depth estimation time
                depth_estimation_stop_tick = xTaskGetTickCount() - depth_estimation_start_tick;


                depth_estimation_data.depth_estimation_time_ms = depth_estimation_stop_tick * (1000 / configTICK_RATE_HZ);

                printf("Depths for detections: ");
                for (uint8_t i = 0; i < detection_data.detection_count; i++) {
                    printf("%f ", depth_estimation_data.depths[i]);
                }   
                printf("\r\n");
            }

        }


        // Output logging data structure to queue
        logging_data.timestamp_ms = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
        logging_data.system_state = current_state;
        logging_data.detection_data = detection_data;
        logging_data.depth_estimation_data = depth_estimation_data;
        if (xQueueOverwrite(g_logging_queue_m7, &logging_data) != pdTRUE) {
            printf("ERROR: Failed to send logging data\r\n");
        }
    }


    void state_logic_host_disconnected(SystemState& current_state, SystemState& new_state, DetectionData& detection_data, DepthEstimationData& depth_estimation_data, VL53L8CX_ResultsData& tof_data, LoggingData& logging_data) {

        TickType_t depth_estimation_start_tick;
        TickType_t depth_estimation_stop_tick;

        new_state = SystemState::SCANNING;


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
                depth_estimation_start_tick = xTaskGetTickCount();

                depth_estimation_data.timestamp_ms = depth_estimation_start_tick * (1000 / configTICK_RATE_HZ);

                // Perform depth estimation
                depth_estimation(
                    detection_data.detections, 
                    detection_data.detection_count, 
                    tof_data.distance_mm, 
                    depth_estimation_data.depths
                );

                // Update depth estimation time
                depth_estimation_stop_tick = xTaskGetTickCount() - depth_estimation_start_tick;


                depth_estimation_data.depth_estimation_time_ms = depth_estimation_stop_tick * (1000 / configTICK_RATE_HZ);

                printf("Depths for detections: ");
                for (uint8_t i = 0; i < detection_data.detection_count; i++) {
                    printf("%f ", depth_estimation_data.depths[i]);
                }   
                printf("\r\n");


                // Check if any depth is below the danger distance threshold
                for (uint8_t i = 0; i < detection_data.detection_count; i++) {
                    if (depth_estimation_data.depths[i] <= danger_depth_mm) {

                        new_state = SystemState::STOPPED;

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
        logging_data.timestamp_ms = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
        logging_data.system_state = current_state;
        logging_data.detection_data = detection_data;
        logging_data.depth_estimation_data = depth_estimation_data;
        if (xQueueOverwrite(g_logging_queue_m7, &logging_data) != pdTRUE) {
            printf("ERROR: Failed to send logging data\r\n");
        }

    }


    void state_controller_task(void* parameters) {
        (void)parameters;
        printf("State controller task starting...\r\n");
        
        SystemState current_state = SystemState::UNINITIALIZED;
        
        // Update to idle ( Host reading) if uninitialized
        if (current_state == SystemState::UNINITIALIZED) {
            current_state = SystemState::HOST_READING;
            xQueueOverwrite(g_state_update_queue_m7, &current_state);
        }
        
        // Setup static data structures to prevent stack overflow
        static DetectionData detection_data;
        static DepthEstimationData depth_estimation_data;
        static VL53L8CX_ResultsData tof_data;
        static LoggingData logging_data;

        static HostConnectionStatus host_condition = HostConnectionStatus::DISCONNECTED;

        static HostState host_state = HostState::UNDEFINED;
        
        // Initialize the system state
        SystemState new_state = current_state;

        
        while (true) {
            // Default next state is current state
            new_state = current_state;
            new_state = SystemState::HOST_READING;

            // Update to latest host connection status
            xQueueReceive(g_host_connection_status_queue_m7, &host_condition, 0);

            // If host connected, do host connection logic
            if (host_condition == HostConnectionStatus::CONNECTED){
                state_logic_host_connected(
                    host_state, 
                    current_state, 
                    new_state, 
                    detection_data, 
                    depth_estimation_data, 
                    tof_data, 
                    logging_data
                );
            }

            else {
                state_logic_host_disconnected(
                    current_state, 
                    new_state, 
                    detection_data, 
                    depth_estimation_data, 
                    tof_data, 
                    logging_data
                );
            }


            // Task delay
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

}