
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"


#include "m7/task_config_m7.hh"
#include "m7/m7_queues.hh"

#include "logo.hh"


namespace coralmicro {
namespace {

const char* PROJECT_NAME = "PCB Bringup";

void setup_tasks() {
    printf("Starting M7 task creation...\r\n");

    // Init queues
    // BOOL return type
    if (!InitQueues()) {
        printf("Failed to initialize queues\r\n");
        vTaskSuspend(nullptr);
    }
    
    // Task creation
    // TaskErr_t return type
    if (CreateM7Tasks() != TaskErr_t::OK)
    {
        printf("Failed to create M7 tasks\r\n");
        vTaskSuspend(nullptr);
    }
}

[[noreturn]] void main_m7() {
    // Print startup banner
    printf("\n%s\r\n", PROJECT_NAME);
    printf("Developed by JC \r\n");
    printf("%s\r\n\n", PROJECT_LOGO);

    // Initialize M7 tasks
    setup_tasks();

    printf("Entering M7 main loop\r\n");
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