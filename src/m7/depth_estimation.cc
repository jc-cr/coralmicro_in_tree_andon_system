// depth_estimation.cc
#include "m7/depth_estimation.hh"

namespace coralmicro {
    void depth_estimation(
        const tensorflow::Object* detections, // array of detections
        const uint8_t detection_count,    // number of detections 
        const int16_t* distance_mm,   //  array from ToF
        const uint8_t tof_resolution,       // Current ToF resolution (4x4 or 8x8)
        float* depths_out
    ){
        
        // Check valid inputs
        if (!detections || detection_count == 0 || !distance_mm || !depths_out) {
            return;
        }
        
        
        // Process each detection
        for (uint8_t i = 0; i < detection_count; i++) {
            // Get depth value directly from the distance array
            depths_out[i] = 2; // DEBUG: 2 is a placeholder, replace with actual logic to calculate depth from distance_mm }
        }
    }

} // namespace coralmicro