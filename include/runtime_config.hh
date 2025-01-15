// runtime_config.hh
// Updateable by RPC user computer, defines the runtime configuration of the system

#pragma once

namespace coralmicro{

    struct RuntimeConfig{

        unsigned float estop_activation_distance_mm = 500.0f;
        unsigned float person_detection_min_confidence = 0.5f;

        unsigned int stopped_state_color = 0xFF0000;
        unsigned int running_state_color = 0x00FF00;
        unsigned int warning_state_color = 0xFFFF00;

    }
}