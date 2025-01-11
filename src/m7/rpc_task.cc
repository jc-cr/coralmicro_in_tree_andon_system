// rpc_task.cc
#include "m7/rpc_task.hh"

namespace coralmicro {

    void get_frame(struct jsonrpc_request* request) {
        CameraData camera_data;
        
        if (xQueuePeek(g_ipc_camera_queue_m7, &camera_data, 0) != pdTRUE) {
            jsonrpc_return_error(request, -1, "No camera data available", nullptr);
            return;
        }

        jsonrpc_return_success(
            request,
            "{%Q: %d, %Q: %d, %Q: %V}",
            "width", camera_data.width,
            "height", camera_data.height,
            "base64_data", camera_data.image_data->size(), camera_data.image_data->data()
        );
    }

    void get_tof_grid(struct jsonrpc_request* request) {
        VL53L8CX_ResultsData results;
        
        // Try to get latest TOF frame without waiting
        if (xQueuePeek(g_tof_queue_m7, &results, 0) != pdTRUE) {
            jsonrpc_return_error(request, -1, "No TOF data available", nullptr);
            return;
        }

        // Start building the distances array
        std::string distances_array = "[";
        for (int i = 0; i < 64; i++) {  // 8x8 grid = 64 elements
            if (results.nb_target_detected[i] > 0) {
                distances_array += std::to_string(results.distance_mm[i]);
            } else {
                distances_array += "0";  // No target detected
            }
            if (i < 63) distances_array += ",";
        }
        distances_array += "]";

        // Return as a JSON object with the distances array and temperature
        jsonrpc_return_success(
            request,
            "{%Q: %s, %Q: %d}",
            "distances", distances_array.c_str(),
            "temperature", results.silicon_temp_degc
        );
    }

    void rpc_task(void* parameters) {
        (void)parameters;
        
        printf("RPC task starting...\r\n");
        
        std::string usb_ip;
        if (!GetUsbIpAddress(&usb_ip)) {
            printf("Failed to get USB IP Address\r\n");
            vTaskSuspend(nullptr);
        }
        printf("Starting Stream Service on: %s\r\n", usb_ip.c_str());

        jsonrpc_init(nullptr, nullptr);
        jsonrpc_export("get_image_from_camera", get_frame);
        jsonrpc_export("get_tof_grid", get_tof_grid);  // Register TOF endpoint
        UseHttpServer(new JsonRpcHttpServer);

        vTaskSuspend(nullptr);
    }
}