// system_enums.hh
#pragma once

enum class HostState {
    // PACKML states from ROS packml
    UNDEFINED, // Red
    STOPPED, // Red
    STARTING, // Green
    IDLE, // Blue
    SUSPENDED, // Yellow
    EXECUTE, // Green
    STOPPING, // Red
    ABORTING, // Red
    ABORTED, // Red
    HOLDING, // Yellow
    HELD, // Yellow
    RESETTING, // Blue
    SUSPENDING, // Yellow
    UNSUSPENDING,
    CLEARING, // Red
    UNHOLDING, // Yellow
    COMPLETING, // Blue
    COMPLETE, // Blue
};


struct RedStates {
    static constexpr HostState UNDEFINED = HostState::UNDEFINED;
    static constexpr HostState STOPPED = HostState::STOPPED;
    static constexpr HostState STOPPING = HostState::STOPPING;
    static constexpr HostState ABORTING = HostState::ABORTING;
    static constexpr HostState ABORTED = HostState::ABORTED;
    static constexpr HostState CLEARING = HostState::CLEARING;

    static bool Contains(HostState state) {
        return state == UNDEFINED || state == STOPPED || state == STOPPING || 
               state == ABORTING || state == ABORTED || state == CLEARING;
    }
};

struct GreenStates {
    static constexpr HostState STARTING = HostState::STARTING;
    static constexpr HostState EXECUTE = HostState::EXECUTE;

    static bool Contains(HostState state) {
        return state == STARTING || state == EXECUTE;
    }
};

struct YellowStates {
    static constexpr HostState SUSPENDED = HostState::SUSPENDED;
    static constexpr HostState HOLDING = HostState::HOLDING;
    static constexpr HostState HELD = HostState::HELD;
    static constexpr HostState UNHOLDING = HostState::UNHOLDING;

    static bool Contains(HostState state) {
        return state == SUSPENDED || state == HOLDING || 
               state == HELD || state == UNHOLDING;
    }
};

struct BlueStates {
    static constexpr HostState IDLE = HostState::IDLE;
    static constexpr HostState RESETTING = HostState::RESETTING;
    static constexpr HostState COMPLETING = HostState::COMPLETING;
    static constexpr HostState COMPLETE = HostState::COMPLETE;

    static bool Contains(HostState state) {
        return state == IDLE || state == RESETTING || 
               state == COMPLETING || state == COMPLETE;
    }
};

enum class SystemState {
    UNINITIALIZED,
    HOST_READING,
    SCANNING,
    STOPPED,
    WARNING,
    IDLE,
    ACTIVE,
};

enum class HostConnectionStatus {
    DISCONNECTED,
    CONNECTED,
};

enum class OperatingMode {
    HOST_DEPENDENT, // Implement later
    HOST_INDEPENDENT, // Implement later
    MIXED
};