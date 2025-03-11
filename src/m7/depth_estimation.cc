// depth_estimation.cc
#include "m7/depth_estimation.hh"

namespace coralmicro {

    // Helper function to find cells that overlap with bounding box
    std::vector<uint8_t> find_overlapping_cells(const tensorflow::Object& detection, const CameraData& camera_data) {
        std::vector<uint8_t> overlapping_cells; // Just the cell indices
        
        // Convert normalized coordinates to pixel coordinates
        uint16_t x_min = static_cast<uint16_t>(detection.bbox.xmin * camera_data.width);
        uint16_t y_min = static_cast<uint16_t>(detection.bbox.ymin * camera_data.height);
        uint16_t x_max = static_cast<uint16_t>(detection.bbox.xmax * camera_data.width);
        uint16_t y_max = static_cast<uint16_t>(detection.bbox.ymax * camera_data.height);
        
        // Clamp to image boundaries
        x_min = std::min(x_min, static_cast<uint16_t>(camera_data.width - 1));
        y_min = std::min(y_min, static_cast<uint16_t>(camera_data.height - 1));
        x_max = std::min(x_max, static_cast<uint16_t>(camera_data.width - 1));
        y_max = std::min(y_max, static_cast<uint16_t>(camera_data.height - 1));
        
        // Check each cell for overlap
        for (uint8_t cell_idx = 0; cell_idx < kTofCellCount; cell_idx++) {
            const auto& cell_region = kTofCellRegions[cell_idx];
            
            // Check if the cell and bounding box overlap
            if (rectangles_overlap(
                    x_min, y_min, x_max, y_max,
                    cell_region.x_min, cell_region.y_min, cell_region.x_max, cell_region.y_max)) {
                
                // Add this cell to our list
                overlapping_cells.push_back(cell_idx);
            }
        }
        
        return overlapping_cells;
    }

    // Compute depth for a detection using simple averaging with outlier removal
    float compute_depth(const tensorflow::Object& detection, 
                       const VL53L8CX_ResultsData& tof_data,
                       const CameraData& camera_data) {
        // Find cells that overlap with the detection
        std::vector<uint8_t> cells = find_overlapping_cells(detection, camera_data);
        
        if (cells.empty()) {
            return -1.0f; // No overlapping cells
        }
        
        // Collect valid depth values
        std::vector<uint16_t> depths;
        
        for (uint8_t cell_idx : cells) {
            // Skip invalid readings
            uint16_t depth = tof_data.distance_mm[cell_idx];
            if (depth > 0 && depth < 4000) { // Valid range (0-4m)
                depths.push_back(depth);
            }
        }
        
        if (depths.empty()) {
            return -1.0f; // No valid depth readings
        }
        
        // Basic outlier removal if we have enough measurements
        if (depths.size() > 2) {
            // Get median depth
            std::vector<uint16_t> sorted_depths = depths;
            std::sort(sorted_depths.begin(), sorted_depths.end());
            uint16_t median = sorted_depths[sorted_depths.size() / 2];
            
            // Remove values that deviate significantly from median
            const float threshold = 0.25f; // 25% threshold
            auto new_end = std::remove_if(depths.begin(), depths.end(),
                [median, threshold](uint16_t depth) {
                    return std::abs(static_cast<float>(depth) - median) > (median * threshold);
                });
            
            depths.erase(new_end, depths.end());
        }
        
        if (depths.empty()) {
            return -1.0f; // All values were outliers
        }
        
        // Simple average calculation
        float sum = 0.0f;
        for (uint16_t depth : depths) {
            sum += depth;
        }
        
        return sum / depths.size();
    }

    DepthEstimationData depth_estimation(DetectionData& detection_data) {
        DepthEstimationData output_data;
        VL53L8CX_ResultsData tof_data;
        
        // Start timing
        TickType_t start_time = xTaskGetTickCount();
        
        // Get latest TOF data from queue - needed for depth calculation
        if (xQueuePeek(g_tof_queue_m7, &tof_data, 0) == pdTRUE) {
            // Clear previous results
            output_data.detection_depths->clear();
            
            for (const auto& detection : *(detection_data.detections)) {
                DetectionDepth result;
                result.detection = detection;  // Store the entire detection object
                result.depth_mm = compute_depth(detection, tof_data, detection_data.camera_data);
                result.valid = (result.depth_mm > 0);
                
                if (result.valid) {
                    printf("Detection (id=%d) depth: %.1f mm\r\n", 
                        detection.id, result.depth_mm);
                }
                
                output_data.detection_depths->push_back(result);  // Use arrow operator
            }
            
            // Set metadata
            output_data.camera_data = detection_data.camera_data;
            output_data.timestamp = xTaskGetTickCount();
            output_data.detection_data_inference_time = detection_data.inference_time;
            output_data.depth_estimation_time = xTaskGetTickCount() - start_time;
        }
        
        return output_data;
    }

} // namespace coralmicro