// global_config.hh
#pragma once

#include <atomic>
#include <memory>

extern "C" {
#include "vl53l8cx_api.h"
}

#include "system_states.hh"
#include "libs/tpu/edgetpu_manager.h"

// Global configuration values
namespace coralmicro {

    // State Controller Settings
    inline std::atomic<SystemState> g_system_state{SystemState::UNINITIALIZED};

    // TOF config
    inline std::atomic<uint8_t> g_tof_resolution{VL53L8CX_RESOLUTION_4X4};  // Default to 4x4

    // Model config
    inline std::vector<uint8_t> g_model_data;
    constexpr int g_tensor_arena_size = 8 * 1024 * 1024;
    inline uint8_t* g_tensor_arena = nullptr;
    inline char const* g_model_path = "/apps/coralmicro_in_tree_andon_system/models/tf2_ssd_mobilenet_v2_coco17_ptq_edgetpu.tflite";
    inline const float g_threshold = 0.5f; 

    // TPU context (global to keep alive between tasks)
    inline EdgeTpuManager* g_tpu_manager_singleton = nullptr;

}