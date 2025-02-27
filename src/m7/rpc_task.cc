// rpc_task.cc
#include "rpc_task.hh"
namespace coralmicro {

    void tx_logs_to_host(struct jsonrpc_request* request) {
        (void)request;

    }

    void rx_from_host(struct jsonrpc_request* request) {
        (void)request;

        // State update using the state enum
        // acts as a hearbeat between the host and the device
        // if not valid state received for 1 minute or on start up
        // then host condition set to UNCONNECTED

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
        jsonrpc_export("tx_logs_to_host", tx_logs_to_host);
        jsonrpc_export("rx_from_host", rx_from_host);
        
        // Create HTTP server with custom configuration
        auto server = new JsonRpcHttpServer();
        UseHttpServer(server);

        printf("RPC server ready\r\n");
        vTaskSuspend(nullptr);
    }

}