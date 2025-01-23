#include "m7/depth_estimation_task.hh"

namespace coralmicro {

    bool is_pixel_in_bbox(const PixelCoord& coord, const tensorflow::BBox& bbox, 
                        uint32_t image_width, uint32_t image_height) {
        float pixel_x = static_cast<float>(coord.x) / image_width;
        float pixel_y = static_cast<float>(coord.y) / image_height;
        
        return (pixel_x >= bbox.xmin && pixel_x <= bbox.xmax &&
                pixel_y >= bbox.ymin && pixel_y <= bbox.ymax);
    }

    // In get_overlapping_depth_cells:
    for (const auto& coord : kTofRgbMap[cell_idx].pixels) {
        if (is_pixel_in_bbox(coord, bbox, image_width, image_height)) {
            has_overlap = true;
            break;
        }
    }

    // Get valid depth cells that overlap with a bounding box
    std::vector<DepthCell> get_overlapping_depth_cells(
        const tensorflow::BBox& bbox,
        const VL53L8CX_ResultsData& tof_data,
        uint32_t image_width,
        uint32_t image_height) {
        
        std::vector<DepthCell> overlapping_cells;
        
        // For each TOF cell
        for (size_t cell_idx = 0; cell_idx < kTofCellCount; ++cell_idx) {
            // Check if cell has valid measurement
            if (tof_data.nb_target_detected[cell_idx] == 0 || 
                tof_data.target_status[cell_idx] != 5) {  // Status 5 indicates valid measurement
                continue;
            }
            
            // Check if any pixels in this cell overlap with bbox
            bool has_overlap = false;
            for (uint32_t pixel_idx : kTofRgbMap[cell_idx].pixel_indices) {
                auto [x, y] = pixel_to_coords(pixel_idx, image_width);
                if (is_pixel_in_bbox(x, y, bbox, image_width, image_height)) {
                    has_overlap = true;
                    break;
                }
            }
            
            if (has_overlap) {
                overlapping_cells.push_back({
                    tof_data.distance_mm[cell_idx],
                    tof_data.target_status[cell_idx],
                    true
                });
            }
        }
        
        return overlapping_cells;
    }

    // Calculate depth estimate using histogram approach
    uint16_t calculate_depth_estimate(const std::vector<DepthCell>& cells) {
        if (cells.empty()) return 0;
        
        // Create histogram of distances
        std::unordered_map<uint16_t, uint32_t> histogram;
        for (const auto& cell : cells) {
            histogram[cell.distance_mm]++;
        }
        
        // Sort distances
        std::vector<uint16_t> distances;
        distances.reserve(histogram.size());
        for (const auto& entry : histogram) {
            distances.push_back(entry.first);
        }
        std::sort(distances.begin(), distances.end());
        
        // Take average of closest 33% of measurements
        size_t num_samples = std::max(size_t(1), distances.size() / 3);
        uint32_t sum = 0;
        uint32_t count = 0;
        
        for (size_t i = 0; i < num_samples && i < distances.size(); ++i) {
            sum += distances[i] * histogram[distances[i]];
            count += histogram[distances[i]];
        }
        
        return count > 0 ? static_cast<uint16_t>(sum / count) : 0;
    }

    void depth_estimation_task(void* parameters) {
        (void)parameters;
        printf("Depth estimation task starting...\r\n");
        
        // Initialize output queue
        if (!InitDepthEstimationQueue()) {
            printf("Failed to create depth estimation output queue\r\n");
            vTaskSuspend(nullptr);
            return;
        }
        
        InferenceData latest_inference_data;
        TofData latest_tof_data;
        DepthEstimationData depth_data;
        
        while (true) {
            bool have_inference = false;
            bool have_tof = false;
            
            // Try to get latest inference results
            if (xQueuePeek(g_inference_output_queue_m7, &latest_inference_data, 0) == pdTRUE) {
                have_inference = true;
            }
            
            // Try to get latest TOF data
            if (xQueuePeek(g_tof_queue_m7, &latest_tof_data, 0) == pdTRUE) {
                have_tof = true;
            }
            
            // Process if we have both types of data
            if (have_inference && have_tof) {
                // Clear previous person depths
                depth_data.person_depths.clear();
                depth_data.inference_data = latest_inference_data;
                depth_data.tof_data = latest_tof_data;
                depth_data.timestamp = xTaskGetTickCount();
                
                // For each detected person
                for (const auto& detection : latest_inference_data.results) {
                    // Get overlapping depth cells
                    auto overlapping_cells = get_overlapping_depth_cells(
                        detection.bbox,
                        latest_tof_data.tof_results,
                        latest_inference_data.camera_data.width,
                        latest_inference_data.camera_data.height
                    );
                    
                    // Calculate depth estimate
                    uint16_t depth = calculate_depth_estimate(overlapping_cells);
                    
                    if (depth > 0) {
                        // Store just the ID and depth for this person
                        depth_data.person_depths.push_back({detection.id, depth});
                        printf("Person ID %d detected at depth: %u mm\r\n", 
                            detection.id, depth);
                    }
                }
                
                // Send updated depth estimation data to queue
                if (xQueueOverwrite(g_depth_estimation_output_queue_m7, &depth_data) != pdTRUE) {
                    printf("Failed to send depth estimation data to queue\r\n");
                }
            }
            
            vTaskDelay(pdMS_TO_TICKS(33));  // ~30 FPS
        }
    }

} // namespace coralmicro