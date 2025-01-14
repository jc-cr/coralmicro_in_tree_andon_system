// tof_task.hh
#pragma once

// Coral Micro
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"
#include "libs/base/i2c.h"
#include "libs/base/gpio.h"

// VL53L8CX implementation
extern "C" {
#include "vl53l8cx_api.h"
}

#include "platform.hpp"

// C++ standard library
#include <stdio.h>
#include <memory>

#include "m7/m7_queues.hh"

namespace coralmicro {
    // Task
    void tof_task(void* parameters);

    // Initialization
    bool init_sensor(VL53L8CX_Configuration* dev);
    bool init_gpio();

    // Helper functions
    const char* get_error_string(uint8_t status);
    void print_sensor_error(const char* operation, uint8_t status);
    void print_results(VL53L8CX_ResultsData* results);


    // Constants
    struct TofTaskQueues {
        static constexpr QueueHandle_t* output_queue = &g_tof_queue_m7;
    };


    static constexpr Gpio kLpnPin = Gpio::kPwm0;
    static constexpr I2c kI2c = I2c::kI2c1;
    
    static constexpr uint16_t kAddress = 0x29; // 0x58 >> 1
    static constexpr uint8_t kResolution = VL53L8CX_RESOLUTION_8X8;
    static constexpr uint8_t kRangingFrequency = 15; // Hz
    static constexpr uint8_t kIntegrationTime = 10;  // ms
}
