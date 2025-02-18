// rpc_task.cc
#include "rpc_task.hh"
namespace coralmicro {
    
    void get_tof_grid(struct jsonrpc_request* request) {
        VL53L8CX_ResultsData results;
        
        // Try to get latest TOF frame without waiting
        if (xQueuePeek(g_tof_queue_m7, &results, 0) != pdTRUE) {
            jsonrpc_return_error(request, -1, "No TOF data available", nullptr);
            return;
        }

        // Get current mode value
        uint8_t current_mode = g_tof_resolution.load();

        jsonrpc_return_success(
            request,
            "{%Q: %d, %Q: %V}",
            "mode", current_mode,
            "results", sizeof(VL53L8CX_ResultsData), &results
        );
    }


    // Cache the last successful frame to reduce queue access
    static CameraData cached_frame;
    static bool has_cached_frame = false;
    static TickType_t last_frame_time = 0;

    void get_frame(struct jsonrpc_request* request) {
        // First check if we have a recent cached frame (less than 100ms old)
        TickType_t current_time = xTaskGetTickCount();
        bool need_new_frame = !has_cached_frame || 
                            (current_time - last_frame_time) > pdMS_TO_TICKS(100);
        
        if (need_new_frame) {
            // Try to get a new frame from the queue
            if (xQueuePeek(g_camera_queue_m7, &cached_frame, 0) != pdTRUE) {
                jsonrpc_return_error(request, -1, "No camera data available", nullptr);
                return;
            }
            
            if (!cached_frame.image_data || cached_frame.image_data->empty()) {
                jsonrpc_return_error(request, -2, "Camera data is empty", nullptr);
                return;
            }

            has_cached_frame = true;
            last_frame_time = current_time;
        }

        // Use the cached frame
        jsonrpc_return_success(
            request,
            "{%Q: %d, %Q: %d, %Q: %V}",
            "width", cached_frame.width,
            "height", cached_frame.height,
            "base64_data", cached_frame.image_data->size(), 
            cached_frame.image_data->data()
        );
    }

    void rpc_task(void* parameters) {
        (void)parameters;
        
        printf("RPC task starting with increased priority...\r\n");


        
        std::string usb_ip;
        if (!GetUsbIpAddress(&usb_ip)) {
            printf("Failed to get USB IP Address\r\n");
            vTaskSuspend(nullptr);
        }
        printf("Starting Stream Service on: %s\r\n", usb_ip.c_str());

        // Initialize RPC server with increased timeout
        jsonrpc_init(nullptr, nullptr);
        jsonrpc_export("get_image_from_camera", get_frame);
        jsonrpc_export("get_tof_grid", get_tof_grid);
        
        // Create HTTP server with custom configuration
        auto server = new JsonRpcHttpServer();
        UseHttpServer(server);

        // Reset frame cache state
        has_cached_frame = false;
        last_frame_time = 0;

        printf("RPC server ready (high priority)\r\n");
        vTaskSuspend(nullptr);
    }

}