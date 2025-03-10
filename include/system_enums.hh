// system_enums.hh
#pragma once

enum class HostState {
    // Host ot use PACKML state defintions
    UNDEFINED,
    // Inactive states
    STOPPED,
    // Active states
    STARTING,
    IDLE,
    SUSPENDED,
    EXECUTE,
    // Inactive states
    STOPPING,
    // Aborting states
    ABORTING,
    ABORTED,
    // Active states
    HOLDING,
    HELD,
    RESETTING,
    SUSPENDING,
    UNSUSPENDING,
    // Inactive states
    CLEARING,
    // Active states
    UNHOLDING,
    COMPLETING,
    COMPLETE,
};

enum class SystemState {
    UNINITIALIZED,
    HOST_READING,
    SCANNING,
    WARNING,
    STOPPED,
    HOST_ACTIVE_STATE,
    HOST_INACTIVE_STATE,
    HOST_ABORTING
};

enum class HostConnectionStatus {
    UNCONNECTED,
    CONNECTED,
    ERROR,
};

enum class SystemFault {
    NO_FAULT,
    ESTOPPED,
    PROGRAM_ERROR
};