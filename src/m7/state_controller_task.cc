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

        SystemState new_state = current_state;

        // Setup host connection condition
        // Initialize last connection time
        TickType_t last_connection_time = xTaskGetTickCount();

        HostConnectionStatus current_host_connection_status = HostConnectionStatus::UNCONNECTED;

        HostConnectionStatus new_host_connection_status = current_host_connection_status;
        
        // Setup host state
        HostState current_host_state = HostState::UNDEFINED;
        HostState new_host_state = current_host_state;


        // Setup detection data
        DetectionData detection_data;

        // Setup logging data
        // Logging data will merge the depth estimation data
        LoggingData logging_data;

        // Setup system fault condition
        SystemFault system_fault_status = SystemFault::NO_FAULT;

        while (true) {
            // 1st get host condition, this will allow us to update resulting state logic such as for LED handling etc.

            // Always start in HOST_READING state, this gets updated in the loop to reflect accurate state
            current_state = SystemState::HOST_READING;

            set_host_connection_status(current_host_connection_status, last_connection_time);

            set_host_state(current_host_state);

            // Get latest detection data
            get_latest_detection_data(detection_data);

            // Update the internal state
            set_system_state(current_host_connection_status, current_host_state , current_state, detection_data, logging_data);

            
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


    void get_latest_detection_data(DetectionData &detection_data){
        xQueueReceive(g_detection_output_queue_m7, &detection_data, 0);
    }

    void set_host_state(HostState& current_host_state){

        HostState new_host_state = current_host_state;

        // Check if new host state update received from RPC
        if (xQueueReceive(g_host_state_queue_m7, &new_host_state, 0) == pdTRUE) {
            // Update the host state if not same
            if (new_host_state != current_host_state) {
                current_host_state = new_host_state;
            }
        }   
    }

    void set_system_state(HostConnectionStatus& host_connection_status,
        HostState& current_host_state,
        SystemFault& system_fault, 
        SystemState& current_state, 
        DetectionData& detection_data, 
        LoggingData& logging_data){

        SystemState new_state = current_state;

        // Is system fault active? (SystemFault::)
        if(system_fault != SystemFault::NO_FAULT){
            new_state = SystemState::STOPPED;
        }

        else{

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
                        new_state = SystemState::HOST_ACTIVE_STATE;
                        break;
                }

            }
            else{

            }



            // No continue

        }


        // Is system in host in a ABORTING or INACTIVE state?


        // Is a person detected?

          // NO:
          // Is in independent mode?

          // YES:
          // Is person in danger distance?

            

        if (new_state != current_state) {
            // Update data for LED
            xQueueOverwrite(g_state_update_queue_m7, &new_state);
            printf("System State: %i\r\n", static_cast<int>(new_state));
            current_state = new_state;
        }
    }


    void set_logging_data(SystemState& current_state, DetectionData& detection_data, LoggingData& logging_data){
        /*
            Update logging data for transmission
        */
        logging_data.state = current_state;
        // logging_data.detection_data = detection_data;
        xQueueOverwrite(g_logging_queue_m7, &logging_data);
    }

}