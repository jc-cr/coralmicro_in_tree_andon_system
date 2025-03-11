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


        // Setup host connection condition
        // Initialize last connection time
        TickType_t last_connection_time = xTaskGetTickCount();

        HostConnectionStatus current_host_connection_status = HostConnectionStatus::UNCONNECTED;

        // Setup host state
        HostState current_host_state = HostState::UNDEFINED;

        // Setup detection data
        DetectionData detection_data;

        // Setup depth estimation data
        DepthEstimationData depth_estimation_data;

        // Setup logging data
        // Logging data will merge the depth estimation data
        LoggingData logging_data;

        // Setup system fault condition
        SystemFault system_fault_status = SystemFault::NO_FAULT;

        while (true) {
            // 1st get host condition, this will allow us to update resulting state logic such as for LED handling etc.

            set_host_connection_status(current_host_connection_status, last_connection_time);

            set_host_state(current_host_state, system_fault_status);

            // Get latest detection data
            get_latest_detection_data(detection_data);

            // Update the internal state
            set_system_state(current_host_connection_status,
                current_host_state, 
                system_fault_status,
                current_state, 
                detection_data, 
                depth_estimation_data,
                logging_data);
            
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    void set_host_connection_status(HostConnectionStatus& current_status, TickType_t& last_connection_time){
        /*
            If host is connected, set condition to CONNECTED
            If host is not connected, set condition to UNCONNECTED
        */
        TickType_t current_time = xTaskGetTickCount();
        
        HostConnectionStatus new_status = current_status;

        // Check if new connection update received from RPC
        if (xQueueReceive(g_host_connection_status_queue_m7, &new_status, 0) == pdTRUE) {
            // Update the last connection time
            last_connection_time = current_time;
        }

        // If no update in queue then check if connection timeout exceeded
        else if (current_status == HostConnectionStatus::CONNECTED) {
            // Check if we've exceeded the timeout
            if ((current_time - last_connection_time) > kValidConnectionLimitTicks) {
                // Connection timeout - directly change the condition
                new_status = HostConnectionStatus::UNCONNECTED;
                printf("Host connection timeout, setting to UNCONNECTED\r\n");
            }
        }

        // Update the status
        if (new_status != current_status) {
            current_status = new_status;
        }
    }


    void get_latest_detection_data(DetectionData& detection_data){
        xQueueReceive(g_detection_output_queue_m7, &detection_data, 0);
    }

    void set_host_state(HostState& current_host_state, SystemFault& system_fault){

        HostState new_host_state = current_host_state;

        // Check if new host state update received from RPC
        if (xQueueReceive(g_host_state_queue_m7, &new_host_state, 0) == pdTRUE) {
            // Update the host state if not same
            if (new_host_state != current_host_state) {
                current_host_state = new_host_state;
            }

            // If the new state is a Non-stopped state and a system fault is active, clear the fault
            switch (new_host_state){
                case HostState::STARTING:
                case HostState::IDLE:
                case HostState::SUSPENDED:
                case HostState::EXECUTE:
                case HostState::HOLDING:
                case HostState::HELD:
                case HostState::RESETTING:
                case HostState::SUSPENDING:
                case HostState::UNSUSPENDING:
                case HostState::UNHOLDING:
                case HostState::COMPLETING:
                case HostState::COMPLETE:
                    if (system_fault == SystemFault::ESTOPPED){
                        system_fault = SystemFault::NO_FAULT;
                    }
                    break;
                default:
                    break;
            }
        }   
    }

    void set_system_state(HostConnectionStatus& host_connection_status,
        HostState& current_host_state,
        SystemFault& system_fault, 
        SystemState& current_state, 
        DetectionData& detection_data, 
        DepthEstimationData& depth_estimation_data,
        LoggingData& logging_data){

        SystemState new_state = current_state;

        
        // Is host connected?
        if (host_connection_status == HostConnectionStatus::CONNECTED){
            // Check if host is in a one of the stopped states (RED)
            switch (current_host_state){
                case HostState::STOPPED:
                case HostState::STOPPING:
                case HostState::ABORTING:
                case HostState::ABORTED:
                case HostState::CLEARING:
                    new_state = SystemState::HOST_STOPPED_STATE;
                    break;
                default:
                    break;
            }
        }
        else{
            // TODO: add logic
        }

        // Is host stopped?
        if (new_state == SystemState::HOST_STOPPED_STATE){
            new_state = SystemState::STOPPED;

            // Set system fault to ESTOPPED
            system_fault = SystemFault::ESTOPPED;

            // This gets cleared in get_host_state by checking if host is no longer in a stopped state and then clearing the fault
        }


        // Is a sytem fault active?
        if (new_state != SystemState::STOPPED){
            if (system_fault == SystemFault::ESTOPPED){
                new_state = SystemState::STOPPED;
            }
            else{
                new_state = SystemState::SCANNING;
            }
        }

        if (new_state == SystemState::SCANNING){
            // Check if a person has been detected in the latest detection data
            // this done by checking if detection_data.detections is not empty

            if (detection_data.detections && detection_data.detections->size() > 0){
                printf("Person detected, updating state to WARNING\r\n");

                new_state = SystemState::WARNING;
            }
            else{
                // No person detected, update the state to HOST_ACTIVE_STATE since not in a HOST_STOPPED_STATE
                new_state = SystemState::HOST_ACTIVE_STATE;
            }
        }

        // If in WARNING state, check if person in danger distance
        if (new_state == SystemState::WARNING){
            printf("Checking for danger distance\r\n");

            // Get depth estimation data
            depth_estimation_data = depth_estimation(detection_data);

            // Check all detection depth objects if leq than danger depth
            for (const auto& detection_depth : *(depth_estimation_data.detection_depths)){
                if (detection_depth.depth_mm <= g_danger_depth.load()){
                    printf("Person in danger distance, stopping system\r\n");

                    new_state = SystemState::STOPPED;
                    // system fault to stop. Host will get updaded via log
                    system_fault = SystemFault::ESTOPPED;
                    break;
                }
            }
            // If no person in danger distance, we'll leave the WARNING state
        }
            

        // Update if state has changed
        if (new_state != current_state) {
            // Update data for LED
            xQueueOverwrite(g_state_update_queue_m7, &new_state);
            printf("System State: %i\r\n", static_cast<int>(new_state));
            current_state = new_state;
        }


        // Update logging data
        set_logging_data(current_state, depth_estimation_data, logging_data);
    }


    void set_logging_data(SystemState& current_state, DepthEstimationData& depth_estimation_data, LoggingData& logging_data){
        /*
            Update logging data for transmission
        */
        logging_data.state = current_state;
        logging_data.depth_estimation_data = depth_estimation_data;

        // logging_data.detection_data = detection_data;
        xQueueOverwrite(g_logging_queue_m7, &logging_data);
    }

}