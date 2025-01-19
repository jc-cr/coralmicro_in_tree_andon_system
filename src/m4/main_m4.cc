#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"
#include "libs/base/main_freertos_m4.h"

#include "m4/m4_queues.hh"
#include "m4/task_config_m4.hh"

#include "system_state.hh"

namespace coralmicro {
namespace {

void setup_tasks() {
    printf("Starting M4 task creation...\r\n");

    // Init queues
    // BOOL return type
    if (!InitQueues()) {
        printf("Failed to initialize queues\r\n");
        update_state_event_m4(TaskID::M4_MAIN, SystemEvents::BOOT_FAIL);
    }
    
    // Task creation
    // TaskErr_t return type
    if (CreateM4Tasks() != TaskErr_t::OK)
    {
        printf("Failed to create M4 tasks\r\n");
        update_state_event_m4(TaskID::M4_MAIN, SystemEvents::BOOT_FAIL);
    }
}

[[noreturn]] void main_m4() {

    // Initialize M4 tasks
    setup_tasks();

    printf("Entering M4 main loop\r\n");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
} 

} // namespace
} // namespace coralmicro

extern "C" void app_main(void* param) {
    (void)param;
    printf("Starting M4 initialization...\r\n");
    coralmicro::main_m4();

    // Should not reach here as main_m4 is a loop
    vTaskSuspend(nullptr);
}