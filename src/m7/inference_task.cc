#include "m7/inference_task.hh"

namespace coralmicro {


    bool detect_objects(tflite::MicroInterpreter* interpreter, 
                    const CameraData& camera_data,
                    DetectionData* result) {
        if (!result || !camera_data.image_data) return false;
        
        auto* input_tensor = interpreter->input_tensor(0);
        if (!input_tensor) {
            printf("ERROR: Failed to get input tensor in detect_objects\r\n");
            return false;
        }

        std::memcpy(tflite::GetTensorData<uint8_t>(input_tensor), 
                camera_data.image_data->data(), camera_data.image_data->size());
        
        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk) {
            printf("ERROR: Inference failed with status %d\r\n", invoke_status);
            return false;
        }
        
        // Get results after inference is complete with a temporary vector
        std::vector<tensorflow::Object> temp_results = 
            tensorflow::GetDetectionResults(interpreter, kDetectionThreshold, g_max_detections_per_inference);

        // If no results, return
        if (temp_results.empty()) {
            result->detection_count = 0;
            return false;
        }

        // Filter for person class (COCO class ID 0)
        temp_results.erase(
            std::remove_if(temp_results.begin(), temp_results.end(),
                        [](const tensorflow::Object& obj) { return obj.id != 0; }),
            temp_results.end());
        
        // If no results after filter, return
        if (temp_results.empty()) {
            result->detection_count = 0;
            return false;
        }

        // Copy detections to the fixed array
        result->detection_count = 0;
        for (size_t i = 0; i < temp_results.size() && i < g_max_detections_per_inference; i++) {
            result->detections[i] = temp_results[i];
            result->detection_count++;
        }

        return true;
    }

    void inference_task(void* parameters) {
        (void)parameters;
        printf("Inference task starting...\r\n");
        
        // Add delay to ensure TOF task has fully initialized
        vTaskDelay(pdMS_TO_TICKS(200));
        
        // Check if model data is loaded
        if (g_model_data.empty()) {
            printf("ERROR: Model data is empty\r\n");
            vTaskSuspend(nullptr);
        }
        
        // Setup TFLite interpreter with proper error handling
        tflite::MicroErrorReporter error_reporter;
        tflite::MicroMutableOpResolver<3> resolver;
        
        if (resolver.AddDequantize() != kTfLiteOk) {
            printf("ERROR: Failed to add Dequantize op\r\n");
            vTaskSuspend(nullptr);
        }
        if (resolver.AddDetectionPostprocess() != kTfLiteOk) {
            printf("ERROR: Failed to add DetectionPostprocess op\r\n");
            vTaskSuspend(nullptr);
        }
        if (resolver.AddCustom(kCustomOp, RegisterCustomOp()) != kTfLiteOk) {
            printf("ERROR: Failed to add EdgeTPU custom op\r\n");
            vTaskSuspend(nullptr);
        }

        // Create interpreter with error checking
        const tflite::Model* model = tflite::GetModel(g_model_data.data());
        if (model == nullptr) {
            printf("ERROR: Failed to get model from data\r\n");
            vTaskSuspend(nullptr);
        }
        
        // Create interpreter
        tflite::MicroInterpreter interpreter(
            model,
            resolver,
            g_tensor_arena,
            g_tensor_arena_size,
            &error_reporter
        );
        
        // Allocate tensors with retry mechanism
        TfLiteStatus allocate_status = interpreter.AllocateTensors();
        int retry_count = 0;
        const int max_retries = 3;
        
        while (allocate_status != kTfLiteOk && retry_count < max_retries) {
            printf("Retrying tensor allocation (%d/%d)\r\n", retry_count + 1, max_retries);
            vTaskDelay(pdMS_TO_TICKS(100 * (retry_count + 1)));
            allocate_status = interpreter.AllocateTensors();
            retry_count++;
        }
        
        if (allocate_status != kTfLiteOk) {
            printf("ERROR: Failed to allocate tensors after %d attempts\r\n", max_retries);
            vTaskSuspend(nullptr);
        }

        auto* input_tensor = interpreter.input_tensor(0);
        if (!input_tensor) {
            printf("ERROR: Failed to get input tensor\r\n");
            vTaskSuspend(nullptr);
        }

        printf("Inference setup complete. Model input dimensions: %dx%d\r\n",
            input_tensor->dims->data[1], input_tensor->dims->data[2]);
        
    // Main inference loop
        static CameraData camera_data;
        static DetectionData detection_result;
        
        // Inference Hz
        int Hz = 10;
        const TickType_t inference_period = pdMS_TO_TICKS(1000 / Hz);
        TickType_t last_wake_time = xTaskGetTickCount();
        
        while (true) {
            // Try to receive camera data
            if (xQueueReceive(g_camera_queue_m7, &camera_data, 0) == pdTRUE) {
                detection_result.timestamp = xTaskGetTickCount();

                // Copy camera data to detection result
                detection_result.camera_data = camera_data;
                
                // Perform detection
                if (detect_objects(&interpreter, camera_data, &detection_result)) {
                    // Success - detection_count already set in detect_objects
                    detection_result.inference_time = xTaskGetTickCount() - detection_result.timestamp;
                } 
                else {
                    // No detections or error
                    detection_result.detection_count = 0;
                    detection_result.inference_time = xTaskGetTickCount() - detection_result.timestamp;
                }
                
                // Send results to queue regardless of detection success
                if (xQueueOverwrite(g_detection_output_queue_m7, &detection_result) != pdTRUE) {
                    printf("ERROR: Failed to send detection result\r\n");
                }
            }

            // Use vTaskDelayUntil for more consistent timing
            vTaskDelayUntil(&last_wake_time, inference_period);
        }
    }
}