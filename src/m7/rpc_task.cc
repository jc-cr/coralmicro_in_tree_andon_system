#include "rpc_task.hh"

namespace coralmicro {
    // Receive heartbeat from host and update host condition
    void rx_from_host(struct jsonrpc_request* request) {
        // Extract host state from request as an integer
        int host_connection_check = 0;
        
        // Safely parse parameters using mjson with proper error handling
        if (request->params == nullptr) {
            JsonRpcReturnBadParam(request, "Missing parameters", "host_state");
            return;
        }
        
        // Get parameter length for safe handling
        size_t params_len = strlen(request->params);
        
        // Extract host_state parameter with proper boundary checking
        double value = 0;
        if (mjson_get_number(request->params, params_len, "$.host_state", &value)) {
            host_connection_check = (int)value;
            printf("Parsed host_state value: %d\r\n", host_connection_check);
        } else {
            JsonRpcReturnBadParam(request, "Missing or invalid host_state parameter", "host_state");
            return;
        }
        
        // Only continue if connection check is not 0
        if (host_connection_check == 0) {
            jsonrpc_return_error(request, -1, "Invalid connection check value", NULL);
            return;
        }
        
        // Set host condition to CONNECTED and send to state controller via queue
        HostCondition new_condition = HostCondition::CONNECTED;
        if (xQueueOverwrite(g_host_condition_queue_m7, &new_condition) != pdTRUE) {
            jsonrpc_return_error(request, -1, "Failed to update host condition", NULL);
            return;
        }
        
        // Return a clean success response
        jsonrpc_return_success(request, "{}");
    }
    
    // Transmit logging data to host with improved error handling
    void tx_logs_to_host(struct jsonrpc_request* request) {
        // Get the current logging data
        LoggingData logging_data;
        if (xQueuePeek(g_logging_queue_m7, &logging_data, 0) != pdTRUE) {
            // If no logging data available, return an error
            jsonrpc_return_error(request, -1, "No logging data available", NULL);
            return;
        }
        
        // Return the logging data with clean formatting
        jsonrpc_return_success(request, 
            "{%Q:%d}",
            "state", static_cast<int>(logging_data.state)
        );
    }
    
    // RPC task - only responsible for setting up RPC server and callbacks
    void rpc_task(void* parameters) {
        (void)parameters;
        
        printf("RPC task starting...\r\n");
        
        std::string usb_ip;
        if (!GetUsbIpAddress(&usb_ip)) {
            printf("Failed to get USB IP Address\r\n");
            vTaskSuspend(nullptr);
        }
        printf("Starting Stream Service on: %s\r\n", usb_ip.c_str());
        
        // Initialize RPC server
        jsonrpc_init(nullptr, nullptr);
        jsonrpc_export("tx_logs_to_host", tx_logs_to_host);
        jsonrpc_export("rx_from_host", rx_from_host);
        
        // Create HTTP server
        auto server = new JsonRpcHttpServer();
        UseHttpServer(server);
        printf("RPC server ready\r\n");
        
        // Keep task alive to handle RPC requests
        vTaskSuspend(nullptr);
    }
}