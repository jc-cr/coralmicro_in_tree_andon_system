// state_controller_task.hh

#pragma once

#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"


#include "m7/m7_queues.hh"
#include "global_config.hh"
#include "system_enums.hh"
#include "depth_estimation.hh"

namespace coralmicro {

    void state_controller_task(void* parameters);

    // Helper functions
    void set_host_connection_status(HostConnectionStatus& current_host_connection_status, TickType_t& last_connection_time);
    void set_host_state(HostState& current_host_state, SystemFault& system_fault);
    void get_latest_detection_data(DetectionData& detection_data);
    void set_system_state(HostConnectionStatus& host_connection_status,
        HostState& current_host_state,
        SystemFault& system_fault,
        SystemState& current_state,
        DetectionData& detection_data,
        DepthEstimationData& depth_estimation_data,
        LoggingData& logging_data);

    void set_logging_data(SystemState& current_state, DepthEstimationData& depth_estimation_data, LoggingData& logging_data);

    // Timeout limit in ticks - 3 seconds (assuming 1ms tick rate)
    constexpr TickType_t kValidConnectionLimitTicks = pdMS_TO_TICKS(3000); 
}