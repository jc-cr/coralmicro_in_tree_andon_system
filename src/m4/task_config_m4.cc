// AUTO-GENERATED FILE FROM "scripts/generate_tasks.py"
// EDIT AT YOUR OWN RISK.

#include "m4/task_config_m4.hh"
#include <string.h>

// Task implementations
#include "m4/camera_task.hh"
#include "m4/m4_ipc_task.hh"
#include "m4/tof_task.hh"

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
    {
        camera_task,
        "Camera_Task",
        STACK_SIZE_LARGE,
        0,
        TASK_PRIORITY_MEDIUM,
        nullptr
    },
    {
        tof_task,
        "TOF_Task",
        STACK_SIZE_MEDIUM,
        0,
        TASK_PRIORITY_MEDIUM,
        nullptr
    },
    {
        m4_ipc_task,
        "M4_IPC_Task",
        STACK_SIZE_MEDIUM,
        0,
        TASK_PRIORITY_MEDIUM,
        nullptr
    }
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