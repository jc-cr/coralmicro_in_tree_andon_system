// runtime_config.hh
// Updateable by RPC user computer, defines the runtime configuration of the system

// runtime_config.hh
#pragma once
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/semphr.h"

namespace coralmicro {
    struct RuntimeConfig {
        float estop_activation_distance_mm = 500.0f;
        float person_detection_min_confidence = 0.5f;
        unsigned int stopped_state_color = 0xFF0000;
        unsigned int running_state_color = 0x00FF00;
        unsigned int warning_state_color = 0xFFFF00;
    };

    // Global instances with initialization
    inline RuntimeConfig g_runtime_config;
    inline SemaphoreHandle_t g_runtime_config_mutex = xSemaphoreCreateMutex();

    // Thread-safe access functions
    inline bool read_runtime_config(RuntimeConfig& out_config) {
        if (xSemaphoreTake(g_runtime_config_mutex, portMAX_DELAY) == pdTRUE) {
            out_config = g_runtime_config;
            xSemaphoreGive(g_runtime_config_mutex);
            return true;
        }
        return false;
    }

    inline bool update_runtime_config(const RuntimeConfig& new_config) {
        if (xSemaphoreTake(g_runtime_config_mutex, portMAX_DELAY) == pdTRUE) {
            g_runtime_config = new_config;
            xSemaphoreGive(g_runtime_config_mutex);
            return true;
        }
        return false;
    }
}