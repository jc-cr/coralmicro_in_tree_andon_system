// main_m7.cc
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"

#include "startup_banner.hh"

#include "m7/task_config_m7.hh"
#include "m7/m7_queues.hh"
#include "global_config.hh"
#include "m7/tof_task.hh"

namespace coralmicro {
namespace {

    // Properly allocate tensor arena in SDRAM section
    STATIC_TENSOR_ARENA_IN_SDRAM(tensor_arena_buffer, g_tensor_arena_size);

    // Global TPU context to keep it alive
    std::shared_ptr<EdgeTpuContext> g_tpu_context;

    bool load_model() {
        printf("Attempting to load model in main...\r\n");

        g_tensor_arena = tensor_arena_buffer;
        
        // Check if model exists
        if (!LfsFileExists(g_model_path)) {
            printf("ERROR: Model file not found at %s\r\n", g_model_path);
            return false;
        }
        
        // Try to load the model
        if (!LfsReadFile(g_model_path, &g_model_data)) {
            printf("ERROR: Failed to load model file\r\n");
            return false;
        }

        // Add a delay
        vTaskDelay(pdMS_TO_TICKS(100));
        
        return true;
    }

    bool init_tpu() {
        printf("Initializing EdgeTPU...\r\n");

        // Initialize EdgeTPU with max performance and store in global
        g_tpu_context = EdgeTpuManager::GetSingleton()->OpenDevice(PerformanceMode::kMax);
        if (!g_tpu_context) {
            printf("ERROR: Failed to initialize EdgeTPU\r\n");
            return false;
        }

        // Store TPU context in global space
        g_tpu_manager_singleton = EdgeTpuManager::GetSingleton();

        printf("EdgeTPU initialized successfully\r\n");

        // Add a delay to allow the TPU to stabilize
        vTaskDelay(pdMS_TO_TICKS(200));

        return true;
    }

    bool init_tof_device() {

        // Initialize GPIO first
        if (!init_gpio()) {
            printf("Failed to initialize TOF GPIO\r\n");
            return false;
        }

        // Platform initialization with proper cleanup
        VL53L8CX_Platform platform = {};
        if (!vl53l8cx::PlatformInit(&platform, kI2c, kAddress)) {
            printf("Platform initialization failed\r\n");
            return false;
        }


        // Create and initialize device instance
        g_tof_device = std::make_unique<VL53L8CX_Configuration>();
        if (!g_tof_device) {
            printf("Failed to allocate device configuration\r\n");
            return false;
        }

        g_tof_device->platform = platform;

        if (!init_sensor(g_tof_device.get())) {
            printf("Sensor initialization failed\r\n");
            return false;
        }

        // Allocate results structure on heap
        g_tof_results = std::make_unique<VL53L8CX_ResultsData>();
        if (!g_tof_results) {
            printf("Failed to allocate results structure\r\n");
            return false;
        }


         printf("sizeof(VL53L8CX_ResultsData) in main: %u bytes, alignment: %u\r\n",
        sizeof(VL53L8CX_ResultsData),
        alignof(VL53L8CX_ResultsData));


        return true;
    }

    void setup_tasks() {
            printf("Starting M7 task creation...\r\n");
            
            // Task creation - queues already initialized in start_m4()
            if (CreateM7Tasks() != TaskErr_t::OK)
            {
                printf("Failed to create M7 tasks\r\n");
                vTaskSuspend(nullptr);
            }
        }

    [[noreturn]] void main_m7() {
        // Print startup banner
        print_startup_banner();

        // Initialize queues first
        if (!InitQueues()) {
            printf("Failed to initialize queues\r\n");
            vTaskSuspend(nullptr);
        }

        // Add a small delay

        if (!load_model()) {
            printf("Failed to load model\r\n");
            vTaskSuspend(nullptr);
        }

        if (!init_tpu()) {
            printf("Failed to initialize EdgeTPU\r\n");
            vTaskSuspend(nullptr);
        }

        if (!init_tof_device()) {
            printf("Failed to initialize TOF device\r\n");
            vTaskSuspend(nullptr);
        }

        vTaskDelay(pdMS_TO_TICKS(500));

        // Initialize M7 tasks
        setup_tasks();


        while (true) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    } 

} // namespace
} // namespace coralmicro

extern "C" void app_main(void* param) {
    (void)param;
    printf("Starting M7 initialization...\r\n");
    coralmicro::main_m7();

    // Should never reach here as main_m7 is a loop
    vTaskSuspend(nullptr);
}