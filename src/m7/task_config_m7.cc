// AUTO-GENERATED FILE FROM "scripts/generate_tasks.py"
// EDIT AT YOUR OWN RISK.

#include "m7/task_config_m7.hh"
#include <string.h>

// Task implementations
#include "m7/camera_task.hh"
#include "m7/inference_task.hh"
#include "m7/state_controller_task.hh"
#include "m7/tof_task.hh"

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

constexpr TaskConfig kM7TaskConfigs[] = {
    {
        camera_task,
        "Camera_Task",
        STACK_SIZE_MEDIUM,
        0,
        TASK_PRIORITY_MEDIUM,
        nullptr
    },
    {
        tof_task,
        "TOF_Task",
        STACK_SIZE_LARGE,
        0,
        TASK_PRIORITY_MEDIUM,
        nullptr
    },
    {
        inference_task,
        "Inference_Task",
        STACK_SIZE_LARGE,
        0,
        TASK_PRIORITY_MEDIUM,
        nullptr
    },
    {
        state_controller_task,
        "State_Controller_Task",
        STACK_SIZE_LARGE,
        0,
        TASK_PRIORITY_MEDIUM,
        nullptr
    }
};

} // namespace

TaskErr_t CreateM7Tasks() {
    TaskErr_t status = TaskErr_t::OK;

    for (const auto& config : kM7TaskConfigs) {
        BaseType_t ret = xTaskCreate(
            config.taskFunction,
            config.taskName,
            config.stackSize,
            config.parameters,
            config.priority,
            config.handle
        );

        if (ret != pdPASS) {
            printf("Failed to create M7 task: %s\r\n", config.taskName);
            status = TaskErr_t::CREATE_FAILED;
            break;
        }
        
        printf("Created M7 task: %s\r\n", config.taskName);
    }

    return status;
}

} // namespace coralmicro