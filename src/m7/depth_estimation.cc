// depth_estimation.cc
#include "m7/depth_estimation.hh"

namespace coralmicro {

  void depth_estimation(
        const tensorflow::Object* detections, // array of detections
        const uint8_t detection_count,    // number of detections 
        const int16_t* distance_mm,   //  array from ToF
        float* depths_out
    ){
        // Check valid inputs
        if (!detections || detection_count == 0 || !distance_mm || !depths_out) {
            return;
        }
        
        // Process each detection
        for (uint8_t i = 0; i < detection_count; i++) {
            const auto& detection = detections[i];
            
            // Get bounding box coordinates (relative to image dimensions)
            const int detection_x_min = detection.bbox.xmin;
            const int detection_y_min = detection.bbox.ymin;
            const int detection_x_max = detection.bbox.xmax;
            const int detection_y_max = detection.bbox.ymax;
            
            // Variables to track weighted sum of depths
            float total_weighted_depth = 0.0f;
            float total_weight = 0.0f;
            
            // Iterate through all TOF cells to find overlapping regions
            for (uint8_t cell_idx = 0; cell_idx < kTofCellCount; cell_idx++) {
                const auto& cell_region = kTofCellRegions[cell_idx];
                
                // Calculate overlap area between detection bbox and this TOF cell
                uint32_t overlap = overlap_area(
                    detection_x_min, detection_y_min, detection_x_max, detection_y_max,
                    cell_region.x_min, cell_region.y_min, cell_region.x_max, cell_region.y_max
                );
                
                // If there's an overlap, add the weighted depth value
                if (overlap > 0) {
                    // Get the depth value for this cell
                    int16_t cell_depth_mm = distance_mm[cell_idx];
                    
                    // If cell depth is valid (not 0 or negative), include it in the calculation
                    if (cell_depth_mm > 0) {
                        // Combined weight: overlap area * RMSE-based weight
                        float combined_weight = static_cast<float>(overlap) * kTofCellWeights[cell_idx];
                        
                        // Add weighted depth value
                        total_weighted_depth += static_cast<float>(cell_depth_mm) * combined_weight;
                        total_weight += combined_weight;
                    }
                }
            }
            
            // Calculate final depth value
            if (total_weight > 0) {
                depths_out[i] = total_weighted_depth / total_weight;
            }
            else {
                // No valid overlap found, set a default value
                depths_out[i] = -1.0f;  // Negative value indicates invalid measurement
            }
        }
    }
} // namespace coralmicro