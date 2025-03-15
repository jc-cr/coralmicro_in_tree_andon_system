// depth_estimation.cc
#include "m7/depth_estimation.hh"

namespace coralmicro {

    void depth_estimation(const DetectionData& detection_data,
        const VL53L8CX_ResultsData& tof_data,
        DepthEstimationData& depth_estimation_data) {
        // Check if detection data is valid
        if (!detection_data.detections || detection_data.detections->empty()) {
            printf("No detections available for depth estimation!\r\n");
            return;
        }

        // Simply copy the entire detection data struct
        depth_estimation_data.detection_data = detection_data;
        
        // Set timestamps
        depth_estimation_data.timestamp = xTaskGetTickCount();
        
        // Record start time for depth estimation
        TickType_t start_time = xTaskGetTickCount();
        
        // Set placeholder depths - one per detection
        depth_estimation_data.depths.push_back(0.0f);  // Initialize with zero depth
        
        depth_estimation_data.depth_estimation_time = xTaskGetTickCount() - start_time;
        
    }

} // namespace coralmicro