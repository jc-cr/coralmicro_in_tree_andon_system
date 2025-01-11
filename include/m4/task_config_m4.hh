// AUTO-GENERATED FILE BY "scripts/generate_tasks.py" 
// EDIT AT YOUR OWN RISK.

#pragma once

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"

#include <cstdio>

namespace coralmicro {

// Task priorities (configMAX_PRIORITIES = 5)
constexpr int TASK_PRIORITY_HIGH   = (configMAX_PRIORITIES - 1);  // 4
constexpr int TASK_PRIORITY_MEDIUM = (configMAX_PRIORITIES - 2);  // 3
constexpr int TASK_PRIORITY_LOW    = (configMAX_PRIORITIES - 3);  // 2

// Stack sizes
constexpr int STACK_SIZE_LARGE  = (configMINIMAL_STACK_SIZE * 4);  // 1440 bytes
constexpr int STACK_SIZE_MEDIUM = (configMINIMAL_STACK_SIZE * 3);  // 1080 bytes
constexpr int STACK_SIZE_SMALL  = (configMINIMAL_STACK_SIZE * 2);  // 720 bytes

// Task interface
enum class TaskErr_t {
    OK = 0,
    CREATE_FAILED,
};

// Function to create tasks for M4 core
TaskErr_t CreateM4Tasks();

} // namespace coralmicro