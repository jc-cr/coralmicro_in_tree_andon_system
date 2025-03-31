#include "rpc_task.hh"

namespace coralmicro {

    // Receive heartbeat from host and update host condition
    void host_heartbeat(struct jsonrpc_request* request) {
        // Extract host state from request as an integer (mjson_get_bool requires int)
        int host_connection_check;
        
        // Safely parse parameters using mjson with proper error handling
        if (request->params == nullptr) {
            JsonRpcReturnBadParam(request, "Missing parameters", "connected");
            return;
        }
        
        // Get parameter length for safe handling
        size_t params_len = strlen(request->params);
        
        // Parse the 'connected' parameter (not 'host_state')
        if (mjson_get_bool(request->params, params_len, "$.connected", &host_connection_check)) {
        } 
        else {
            JsonRpcReturnBadParam(request, "Missing or invalid connected parameter", "connected");
            return;
        }
        
        if (host_connection_check != 0) {
            // Set host condition to CONNECTED and send to state controller via queue
            HostConnectionStatus new_condition = HostConnectionStatus::CONNECTED;

            if (xQueueOverwrite(g_host_connection_status_queue_m7, &new_condition) != pdTRUE) {
                jsonrpc_return_error(request, -1, "Failed to update host condition", NULL);
                return;
            }
            
            // Return a clean success response
            jsonrpc_return_success(request, "{}");
        }
        else {
            jsonrpc_return_error(request, -1, "Invalid connection check value", NULL);
            return;
        }
    }

    void rx_host_state(struct jsonrpc_request* request) {
        // Extract host state from request as an integer
        double host_state_double;
        
        // Safely parse parameters using mjson with proper error handling
        if (request->params == nullptr) {
            JsonRpcReturnBadParam(request, "Missing parameters", "host_state");
            return;
        }
        
        // Get parameter length for safe handling
        size_t params_len = strlen(request->params);
        
        if (mjson_get_number(request->params, params_len, "$.host_state", &host_state_double)) {
            // Convert double to int to properly interpret as enum
            int host_state_int = static_cast<int>(host_state_double);
            HostState host_state_enum = static_cast<HostState>(host_state_int);
            
            printf("Parsed host_state value: %d (%f)\r\n", host_state_int, host_state_double);
            
            // Send the properly converted host state to the state controller via queue
            if (xQueueOverwrite(g_host_state_queue_m7, &host_state_enum) != pdTRUE) {
                jsonrpc_return_error(request, -1, "Failed to update host state", NULL);
                return;
            }
            
            // Return a clean success response
            jsonrpc_return_success(request, "{}");
        } 
        else {
            JsonRpcReturnBadParam(request, "Missing or invalid host_state parameter", "host_state");
            return;
        }
    }
    
    void tx_logs_to_host(struct jsonrpc_request* request) {
        // Static instance to hold the data
        static LoggingData logging_data;

        if (xQueueReceive(g_logging_queue_m7, &logging_data, 0) != pdTRUE) {
            jsonrpc_return_error(request, -1, "No logging data available", NULL);
            return;
        }
        
        // Calculate actual bytes needed for detection and depth data
        size_t detection_bytes = logging_data.detection_data.detection_count * sizeof(tensorflow::Object);
        size_t depth_bytes = logging_data.detection_data.detection_count * sizeof(float);
        
        // Build response with all the components
        jsonrpc_return_success(request, 
            "{%Q: %d, %Q: %d, %Q: %d, %Q: %d, %Q: %d, %Q: %V, %Q: %V, %Q: %d, %Q: %d, %Q: %d, %Q: %V}",
            "log_timestamp_ms", logging_data.timestamp_ms,
            "system_state", static_cast<int>(logging_data.system_state),
            "detection_count", logging_data.detection_data.detection_count,
            "inference_time_ms", logging_data.detection_data.inference_time_ms,
            "depth_estimation_time_ms", logging_data.depth_estimation_data.depth_estimation_time_ms,
            "detections", detection_bytes, logging_data.detection_data.detections,
            "depths", depth_bytes, logging_data.depth_estimation_data.depths,
            "image_capture_timestamp_ms", logging_data.detection_data.camera_data.timestamp_ms,
            "cam_width", logging_data.detection_data.camera_data.width,
            "cam_height", logging_data.detection_data.camera_data.height,
            "image_data", logging_data.detection_data.camera_data.image_data->size(),  logging_data.detection_data.camera_data.image_data->data()
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

        // Register RPC methods
        jsonrpc_export("host_heartbeat", host_heartbeat);
        jsonrpc_export("rx_host_state", rx_host_state);
        jsonrpc_export("tx_logs_to_host", tx_logs_to_host);

        
        // Create HTTP server
        auto server = new JsonRpcHttpServer();
        UseHttpServer(server);
        printf("RPC server ready\r\n");
        
        // Keep task alive to handle RPC requests
        vTaskSuspend(nullptr);
    }
}