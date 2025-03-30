#include "m7/state_controller_task.hh"

namespace coralmicro{

    // Add detection memory timeouts
    constexpr TickType_t kDetectionMemoryTimeoutMs = 1000;  // 1 second memory for detections
    constexpr TickType_t kTofMemoryTimeoutMs = 1000;        // 1 second memory for TOF data

    void state_logic_host_connected(HostState& host_state, 
        SystemState& current_state, 
        SystemState& new_state, 
        DetectionData& detection_data, DepthEstimationData& depth_estimation_data, 
        VL53L8CX_ResultsData& tof_data, LoggingData& logging_data,
        TickType_t& last_detection_tick, TickType_t& last_tof_tick) {

        bool new_detection_received = false;
        bool new_tof_received = false;
        TickType_t current_tick = xTaskGetTickCount();

        // For host-connected mode, following the "HOST DRIVEN LOGIC" part of diagram
        
        // Get the latest host state
        if (xQueueReceive(g_host_state_queue_m7, &host_state, 0) == pdTRUE) {
            // Host state received successfully
        } 

        // Follow the host-driven logic state transitions according to diagram
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
            // If host is in GREEN state, set system state to ACTIVE
            new_state = SystemState::ACTIVE;
        } 

        // Update system state if it has changed based on host state
        if (new_state != current_state) {
            xQueueOverwrite(g_state_update_queue_m7, &new_state);
            printf("System State: %i\r\n", static_cast<int>(new_state));
            current_state = new_state;
        }

        // After updating system state, always proceed to SCANNING as per diagram
        // to collect detection data for logging purposes (but don't change state)
        
        // Check if a person has been detected in the latest detection data
        if (xQueueReceive(g_detection_output_queue_m7, &detection_data, 10) == pdTRUE) {
            new_detection_received = true;
            if (detection_data.detection_count > 0) {
                last_detection_tick = current_tick; // Update detection timestamp
                printf("New detection received, count: %d\r\n", detection_data.detection_count);
            }
            else {
                printf("Received frame with no detections\r\n");
            }
        }
        else {
            // Check if we still have a valid detection based on timeout
            bool detection_valid = (current_tick - last_detection_tick) <= pdMS_TO_TICKS(kDetectionMemoryTimeoutMs);
            
            if (detection_valid && detection_data.detection_count > 0) {
                printf("Using cached detection data (age: %lu ms, count: %d)\r\n", 
                        (current_tick - last_detection_tick) * (1000 / configTICK_RATE_HZ),
                        detection_data.detection_count);
            }
        }

        TickType_t depth_estimation_start_tick;
        TickType_t depth_estimation_stop_tick;

        if (detection_data.detection_count > 0)
        {
            printf("Checking for danger distance\r\n");
            // Set depth estimation (DE) timestamp
            depth_estimation_start_tick = current_tick;
            depth_estimation_data.timestamp_ms = depth_estimation_start_tick * (1000 / configTICK_RATE_HZ);

            // Get the latest TOF data
            if (xQueueReceive(g_tof_queue_m7, &tof_data, 10) == pdTRUE) {
                new_tof_received = true;
                last_tof_tick = current_tick; // Update TOF timestamp

                // Perform depth estimation
                depth_estimation(
                    detection_data.detections, 
                    detection_data.detection_count, 
                    tof_data.distance_mm, 
                    depth_estimation_data.depths
                );

                printf("Depths for detections: ");
                for (uint8_t i = 0; i < detection_data.detection_count; i++) {
                    printf("%f ", depth_estimation_data.depths[i]);
                }   
                printf("\r\n");
            }
            else {
                // Check if we still have valid TOF data
                bool tof_valid = (current_tick - last_tof_tick) <= pdMS_TO_TICKS(kTofMemoryTimeoutMs);
                
                if (tof_valid) {
                    // Use cached TOF data for decisions
                    printf("Using cached TOF data (age: %lu ms)\r\n", 
                           (current_tick - last_tof_tick) * (1000 / configTICK_RATE_HZ));
                    // We're using cached depth estimation results here
                }
                else {
                    printf("No valid TOF data available\r\n");
                }
            }

            // Update depth estimation time
            depth_estimation_stop_tick = xTaskGetTickCount() - depth_estimation_start_tick;
            depth_estimation_data.depth_estimation_time_ms = depth_estimation_stop_tick * (1000 / configTICK_RATE_HZ);
        }

        // Output logging data structure to queue
        logging_data.timestamp_ms = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
        logging_data.system_state = current_state;
        
        // Only update detection data in logging if we received new data
        if (new_detection_received) {
            logging_data.detection_data = detection_data;
        }
        
        // Only update depth estimation data if we received new TOF data or performed a new estimation
        if (new_tof_received || depth_estimation_data.depth_estimation_time_ms > 0) {
            logging_data.depth_estimation_data = depth_estimation_data;
        }
        
        if (xQueueOverwrite(g_logging_queue_m7, &logging_data) != pdTRUE) {
            printf("ERROR: Failed to send logging data\r\n");
        }
    }

    void state_logic_host_disconnected(SystemState& current_state, SystemState& new_state, 
                                     DetectionData& detection_data, DepthEstimationData& depth_estimation_data, 
                                     VL53L8CX_ResultsData& tof_data, LoggingData& logging_data,
                                     TickType_t& last_detection_tick, TickType_t& last_tof_tick) {
        bool new_detection_received = false;
        bool new_tof_received = false;
        TickType_t depth_estimation_start_tick;
        TickType_t depth_estimation_stop_tick;
        TickType_t current_tick = xTaskGetTickCount();

        // For host-disconnected mode, following the "INDEPENDENT LOGIC" part of diagram
        
        // First transition to SCANNING state as shown in the diagram
        new_state = SystemState::SCANNING;
        
        // Check if a person has been detected in the latest detection data
        if (xQueueReceive(g_detection_output_queue_m7, &detection_data, 10) == pdTRUE) {
            new_detection_received = true;
            if (detection_data.detection_count > 0) {
                // Person detected
                last_detection_tick = current_tick; // Update detection timestamp
                printf("New detection received, count: %d\r\n", detection_data.detection_count);
                
                // Now we need to check if the person is in danger distance
                // to determine if we go to WARNING or STOPPED state
                
                printf("Checking for danger distance\r\n");
                // Set depth estimation (DE) timestamp
                depth_estimation_start_tick = current_tick;
                depth_estimation_data.timestamp_ms = depth_estimation_start_tick * (1000 / configTICK_RATE_HZ);
                
                bool person_in_danger = false;
                
                // Get the latest TOF data
                if (xQueueReceive(g_tof_queue_m7, &tof_data, 10) == pdTRUE) {
                    new_tof_received = true;
                    last_tof_tick = current_tick; // Update TOF timestamp

                    // Perform depth estimation
                    depth_estimation(
                        detection_data.detections, 
                        detection_data.detection_count, 
                        tof_data.distance_mm, 
                        depth_estimation_data.depths
                    );

                    printf("Depths for detections: ");
                    for (uint8_t i = 0; i < detection_data.detection_count; i++) {
                        printf("%f ", depth_estimation_data.depths[i]);
                        
                        // Check if any depth is below the danger distance threshold
                        if (depth_estimation_data.depths[i] <= danger_depth_mm) {
                            person_in_danger = true;
                        }
                    }   
                    printf("\r\n");
                }
                else {
                    // Check if we still have valid TOF data
                    bool tof_valid = (current_tick - last_tof_tick) <= pdMS_TO_TICKS(kTofMemoryTimeoutMs);
                    
                    if (tof_valid) {
                        // Use cached TOF data for decisions
                        printf("Using cached TOF data (age: %lu ms)\r\n", 
                               (current_tick - last_tof_tick) * (1000 / configTICK_RATE_HZ));
                        
                        // Check cached depth estimations for danger distance
                        for (uint8_t i = 0; i < detection_data.detection_count; i++) {
                            if (depth_estimation_data.depths[i] <= danger_depth_mm) {
                                person_in_danger = true;
                            }
                        }
                    }
                    else {
                        printf("No valid TOF data available\r\n");
                    }
                }

                // Update depth estimation time
                depth_estimation_stop_tick = xTaskGetTickCount() - depth_estimation_start_tick;
                depth_estimation_data.depth_estimation_time_ms = depth_estimation_stop_tick * (1000 / configTICK_RATE_HZ);
                
                // Now set the appropriate state based on whether person is in danger
                if (person_in_danger) {
                    new_state = SystemState::STOPPED;
                } else {
                    new_state = SystemState::WARNING;
                }
            }
            else {
                // No person detected in the new frame
                new_state = SystemState::IDLE;
            }
        } 
        else {
            // No new detection data received, check if we have a valid cached detection
            bool detection_valid = (current_tick - last_detection_tick) <= pdMS_TO_TICKS(kDetectionMemoryTimeoutMs);
            
            if (detection_valid && detection_data.detection_count > 0) {
                // We still have a valid detection
                printf("Using cached detection data (age: %lu ms, count: %d)\r\n", 
                       (current_tick - last_detection_tick) * (1000 / configTICK_RATE_HZ),
                       detection_data.detection_count);
                
                // Check if person is in danger distance using cached data
                bool person_in_danger = false;
                
                // Check if we have a valid cached TOF data
                bool tof_valid = (current_tick - last_tof_tick) <= pdMS_TO_TICKS(kTofMemoryTimeoutMs);
                
                if (tof_valid) {
                    // Use cached TOF data for decisions
                    printf("Using cached TOF data (age: %lu ms)\r\n", 
                           (current_tick - last_tof_tick) * (1000 / configTICK_RATE_HZ));
                    
                    // Check cached depth estimations for danger distance
                    for (uint8_t i = 0; i < detection_data.detection_count; i++) {
                        if (depth_estimation_data.depths[i] <= danger_depth_mm) {
                            person_in_danger = true;
                        }
                    }
                    
                    // Set appropriate state based on whether person is in danger
                    if (person_in_danger) {
                        new_state = SystemState::STOPPED;
                    } else {
                        new_state = SystemState::WARNING;
                    }
                }
                else {
                    // No valid TOF data, assume WARNING state
                    new_state = SystemState::WARNING;
                    printf("No valid TOF data available\r\n");
                }
            }
            else {
                // No valid detection data, go to IDLE
                new_state = SystemState::IDLE;
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
        
        // Only update detection data in logging if we received new data
        if (new_detection_received) {
            logging_data.detection_data = detection_data;
        }
        
        // Only update depth estimation data if we received new TOF data or performed a new estimation
        if (new_tof_received || depth_estimation_data.depth_estimation_time_ms > 0) {
            logging_data.depth_estimation_data = depth_estimation_data;
        }
        
        if (xQueueOverwrite(g_logging_queue_m7, &logging_data) != pdTRUE) {
            printf("ERROR: Failed to send logging data\r\n");
        }
    }

    void state_controller_task(void* parameters) {
        (void)parameters;
        printf("State controller task starting...\r\n");
        
        SystemState current_state = SystemState::UNINITIALIZED;
        
        // Update to HOST_READING if uninitialized
        if (current_state == SystemState::UNINITIALIZED) {
            current_state = SystemState::HOST_READING;
            xQueueOverwrite(g_state_update_queue_m7, &current_state);
            printf("System State: %i\r\n", static_cast<int>(current_state));
        }
        
        // Setup static data structures to prevent stack overflow
        static DetectionData detection_data;
        static DepthEstimationData depth_estimation_data;
        static VL53L8CX_ResultsData tof_data;
        static LoggingData logging_data;

        static HostConnectionStatus host_condition = HostConnectionStatus::DISCONNECTED;
        static HostState host_state = HostState::UNDEFINED;
        
        // Add timestamps for detection and TOF memory
        static TickType_t last_detection_tick = 0;
        static TickType_t last_tof_tick = 0;
        
        while (true) {
            // Always start with HOST_READING as per the diagram
            SystemState new_state = SystemState::HOST_READING;
            
            // Update to latest host connection status
            xQueueReceive(g_host_connection_status_queue_m7, &host_condition, 0);

            // If host connected, do host connection logic
            if (host_condition == HostConnectionStatus::CONNECTED) {
                state_logic_host_connected(
                    host_state, 
                    current_state, 
                    new_state, 
                    detection_data, 
                    depth_estimation_data, 
                    tof_data, 
                    logging_data,
                    last_detection_tick,
                    last_tof_tick
                );
            }
            else {
                state_logic_host_disconnected(
                    current_state, 
                    new_state, 
                    detection_data, 
                    depth_estimation_data, 
                    tof_data, 
                    logging_data,
                    last_detection_tick,
                    last_tof_tick
                );
            }

            // Task delay
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}