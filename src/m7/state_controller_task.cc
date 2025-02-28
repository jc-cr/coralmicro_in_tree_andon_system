#include "m7/state_controller_task.hh"

namespace coralmicro{

    void set_system_state(SystemState& old_state, SystemState& new_state, DetectionData& detection_data){
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
            
        if (new_state != old_state) {
            // Update data for LED
            xQueueOverwrite(g_state_update_queue_m7, &new_state);
            printf("System State: %i\r\n", static_cast<int>(new_state));
            old_state = new_state;
        }
    }


    void set_host_condition(HostCondition& old_condition, HostCondition& new_condition, TickType_t& last_connection_time){
        /*
            If host is connected, set condition to CONNECTED
            If host is not connected, set condition to UNCONNECTED
        */
        TickType_t current_time = xTaskGetTickCount();
        
        // Check if new connection update received from RPC
        if (xQueueReceive(g_host_condition_queue_m7, &new_condition, 0) == pdTRUE) {
            // Update the last connection time
            last_connection_time = current_time;
            
            if (new_condition != old_condition) {
                printf("Host Condition: %i\r\n", static_cast<int>(new_condition));
                old_condition = new_condition;
            }
        }
        // If no update in queue then check if connection timeout exceeded
        else if (old_condition == HostCondition::CONNECTED) {
            // Check if we've exceeded the timeout
            if ((current_time - last_connection_time) > kValidConnectionLimitTicks) {
                // Connection timeout - directly change the condition
                old_condition = HostCondition::UNCONNECTED;
                printf("Host connection timeout, setting to UNCONNECTED\r\n");
            }
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

    void state_controller_task(void* parameters) {
        (void)parameters;
        printf("State controller task starting...\r\n");
        
        SystemState current_state = SystemState::UNINITIALIZED;
        // Update to idle if uninitialized
        if (current_state == SystemState::UNINITIALIZED) {
            current_state = SystemState::IDLE;
            xQueueOverwrite(g_state_update_queue_m7, &current_state);
        }
        printf("System State: %i\r\n", static_cast<int>(current_state));

        SystemState new_state = current_state;

        // Setup host condition
        HostCondition current_host_condition = HostCondition::UNCONNECTED;
        printf("Host Condition: %i\r\n", static_cast<int>(current_host_condition));
        HostCondition new_host_condition = current_host_condition;
        
        // Initialize last connection time
        TickType_t last_connection_time = xTaskGetTickCount();
        
        // Setup detection data
        DetectionData detection_data;

        // Setup logging data
        LoggingData logging_data;

        while (true) {
            // 1st get host condition, this will allow us to update resulting state logic such as for LED handling etc.
            set_host_condition(current_host_condition, new_host_condition, last_connection_time);

            // Update the internal state
            set_system_state(current_state, new_state, detection_data);

            // Update logging structure for data tx
            set_logging_data(current_state, detection_data, logging_data);
            
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}