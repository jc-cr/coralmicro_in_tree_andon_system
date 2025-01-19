// system_state.hh
#pragma once


enum class SystemStates : uint8_t {
    BOOTING,
    IDLE,
    RUNNING,
    ERROR,
};

enum class SystemEvents : uint8_t {
    BOOT_FAIL,
    BOOT_SUCCESS,
};

enum class TaskID : uint8_t {
    // M7
    M7_MAIN,
    M7_IPC_TASK,
    M7_INFERENCE_TASK,
    M7_TOF_TASK,
    M7_DEPTH_ESTIMATION_TASK,
    M7_RGB_TASK,
    M7_STATE_CONTROLLER_TASK,
    M7_RPC_TASK,

    // M4
    M4_MAIN,
    M4_IPC_TASK,
    M4_CAMERA_TASK,
};

struct StateEventUpdateMessage {
    TaskID task;
    SystemEvents event;
};

namespace coralmicro {
    void update_state_event_m7(TaskID task, SystemEvents event);
    void update_state_event_m4(TaskID task, SystemEvents event);
} 
