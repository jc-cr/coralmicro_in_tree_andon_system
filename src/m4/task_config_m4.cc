// AUTO-GENERATED FILE FROM "scripts/generate_tasks.py"
// EDIT AT YOUR OWN RISK.

#include "m4/task_config_m4.hh"
#include <string.h>

// Task implementations


namespace coralmicro {
namespace {

struct TaskConfig {
    TaskFunction_t taskFunction;
    const char* taskName;
    uint32_t stackSize;
    void* parameters;
    UBaseType_t priority;
    TaskHandle_t* handle;
};

constexpr TaskConfig kM4TaskConfigs[] = {

};

} // namespace

TaskErr_t CreateM4Tasks() {
    TaskErr_t status = TaskErr_t::OK;

    for (const auto& config : kM4TaskConfigs) {
        BaseType_t ret = xTaskCreate(
            config.taskFunction,
            config.taskName,
            config.stackSize,
            config.parameters,
            config.priority,
            config.handle
        );

        if (ret != pdPASS) {
            printf("Failed to create M4 task: %s\r\n", config.taskName);
            status = TaskErr_t::CREATE_FAILED;
            break;
        }
        
        printf("Created M4 task: %s\r\n", config.taskName);
    }

    return status;
}

} // namespace coralmicro