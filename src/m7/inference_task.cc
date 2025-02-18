#include "m7/inference_task.hh"

namespace coralmicro {

    void inference_task(void* parameters) {
        (void)parameters;
        printf("Inference task starting...\r\n");
        
        // Check if model data is loaded
        if (g_model_data.empty()) {
            printf("ERROR: Model data is empty\r\n");
            vTaskSuspend(nullptr);
        }
        
        
        // Initialize EdgeTPU with max performance
        auto tpu_context = EdgeTpuManager::GetSingleton()->OpenDevice(PerformanceMode::kMax);
        if (!tpu_context) {
            printf("ERROR: Failed to initialize EdgeTPU\r\n");
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

        
        tflite::MicroInterpreter interpreter(
            tflite::GetModel(g_model_data.data()),
            resolver,
            g_tensor_arena,
            g_tensor_arena_size,
            &error_reporter
        );
        
        if (interpreter.AllocateTensors() != kTfLiteOk) {
            printf("ERROR: Failed to allocate tensors\r\n");
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
        CameraData camera_data;
        TickType_t last_inference_time = 0;
        const TickType_t min_inference_period = pdMS_TO_TICKS(30); // Limit to ~30fps
        
        while (true) {
            TickType_t now = xTaskGetTickCount();
            if ((now - last_inference_time) < min_inference_period) {
                vTaskDelay(1);
                continue;
            }

            if (xQueueReceive(g_camera_queue_m7, &camera_data, pdMS_TO_TICKS(100)) == pdTRUE) {
                DetectionData detection_result;
                detection_result.camera_data = camera_data;
                // Timestamp
                TickType_t inference_start_time = xTaskGetTickCount();
                detection_result.timestamp = inference_start_time;
                
                std::vector<tensorflow::Object> detections;
                
                if (detect_objects(&interpreter, camera_data, &detections)) {
                    if (!detections.empty()) {
                        detection_result.detections = std::move(detections);
                        detection_result.inference_time = xTaskGetTickCount() - inference_start_time;

                        // DEBUG: Print detection results from structur including time
                        printf("Inference time: %d ms\r\n", detection_result.inference_time);
                        for (const auto& detection : detection_result.detections) {
                            printf("Detected object ID: %d, Score: %f\r\n", 
                                detection.id, detection.score);
                        }
                        
                        if (xQueueSend(g_detection_output_queue_m7, &detection_result, 0) != pdTRUE) {
                            printf("WARNING: Failed to send detection to depth alignment\r\n");
                        }
                    }
                }
                
                last_inference_time = now;
            }
        }
    }

    bool detect_objects(tflite::MicroInterpreter* interpreter, 
                    const CameraData& camera_data,
                    std::vector<tensorflow::Object>* results) {
        if (!results || !camera_data.image_data) return false;
        
        auto* input_tensor = interpreter->input_tensor(0);
        if (!input_tensor) {
            printf("ERROR: Failed to get input tensor in detect_objects\r\n");
            return false;
        }

        const int model_height = input_tensor->dims->data[1];
        const int model_width = input_tensor->dims->data[2];
        
        if (camera_data.width != model_width || camera_data.height != model_height) {
            printf("ERROR: Image dimensions mismatch: got %dx%d, expected %dx%d\r\n",
                camera_data.width, camera_data.height, model_width, model_height);
            return false;
        }
        
        if (camera_data.image_data->size() < input_tensor->bytes) {
            printf("ERROR: Image data size mismatch: got %zu, expected %zu\r\n",
                camera_data.image_data->size(), input_tensor->bytes);
            return false;
        }
        
        std::memcpy(tflite::GetTensorData<uint8_t>(input_tensor),
                    camera_data.image_data->data(),
                    input_tensor->bytes);
        
        if (interpreter->Invoke() != kTfLiteOk) {
            printf("ERROR: Inference failed\r\n");
            return false;
        }
        
        *results = tensorflow::GetDetectionResults(interpreter, g_threshold, 10);
        
        // Filter for person class (COCO class ID 0)
        results->erase(
            std::remove_if(results->begin(), results->end(),
                        [](const tensorflow::Object& obj) { return obj.id != 0; }),
            results->end());

        if (results->empty()) {
            return false;
        }

        return true;
    }

} // namespace coralmicro