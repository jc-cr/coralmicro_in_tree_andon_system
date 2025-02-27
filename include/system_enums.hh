// system_enums.hh
#pragma once

enum class SystemState {
    UNINITIALIZED,
    IDLE,
    WARNING,
    ERROR,
};

enum class HostCondition {
    UNCONNECTED,
    CONNECTED,
    ERROR,
};