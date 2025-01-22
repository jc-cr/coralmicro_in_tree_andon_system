// rgb_task.hh

#pragma once

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"
#include "libs/base/gpio.h"

#include "m7/m7_queues.hh"
#include "state_definitions.hh"
#include "runtime_config.hh"


#include "third_party/nxp/rt1176-sdk/devices/MIMXRT1176/drivers/fsl_gpt.h"


// External assembly functions
extern "C" void _ZN10coralmicro7SendBitEb(bool bit);
extern "C" void _ZN10coralmicro15InitializeGpioEv();
extern "C" void _ZN10coralmicro10ResetDelayEv();

namespace coralmicro {
    void rgb_task(void* parameters);
}