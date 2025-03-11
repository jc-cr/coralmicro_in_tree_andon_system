// system_enums.hh
#pragma once

enum class HostState {
    // Host ot use PACKML state defintions

    UNDEFINED,
    // Red states
    STOPPED,
    // Green states
    STARTING,
    IDLE,
    SUSPENDED,
    EXECUTE,
    // Red states
    STOPPING,
    // Red states
    ABORTING,
    ABORTED,
    // Green states
    HOLDING,
    HELD,
    RESETTING,
    SUSPENDING,
    UNSUSPENDING,
    // Red states
    CLEARING,
    // Green states
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
    HOST_STOPPED_STATE
};

enum class HostConnectionStatus {
    UNCONNECTED,
    CONNECTED,
};

enum class SystemFault {
    NO_FAULT,
    ESTOPPED,
    PROGRAM_ERROR
};

enum class OperatingMode {
    HOST_DEPENDENT, // Implement later
    HOST_INDEPENDENT, // Implement later
    MIXED
};