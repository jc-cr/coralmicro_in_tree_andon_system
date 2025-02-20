#include "m7/inference_task.hh"

namespace coralmicro {

bool detect_objects(tflite::MicroInterpreter* interpreter, 
                    const CameraData& camera_data,
                    std::vector<tensorflow::Object>* results) {
    if (!results || !camera_data.image_data) return false;
    
    auto* input_tensor = interpreter->input_tensor(0);
    if (!input_tensor) {
        printf("ERROR: Failed to get input tensor in detect_objects\r\n");
        return false;
    }

    std::memcpy(tflite::GetTensorData<uint8_t>(input_tensor), 
            camera_data.image_data->data(), camera_data.image_data->size());
    
    if (interpreter->Invoke() != kTfLiteOk) {
        printf("ERROR: Inference failed\r\n");
        return false;
    }
    
    *results = tensorflow::GetDetectionResults(interpreter, g_threshold, 3);

    // If no results, return
    if (results->empty()) {
        return false;
    }

    // Filter for person class (COCO class ID 1)
    results->erase(
        std::remove_if(results->begin(), results->end(),
                    [](const tensorflow::Object& obj) { return obj.id != 1; }),
        results->end());
    
    // If no results after filter, return
    if (results->empty()) {
        return false;
    }

    // Print person detection
    for (const auto& obj : *results) {
        printf("Detected person with confidence: %.2f\r\n", obj.score);
    }

    return true;
}

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
    
    while (true) {
        // Use receive instead of peek to properly handle the queue
        if (xQueueReceive(g_camera_queue_m7, &camera_data, 0) == pdTRUE) {
            DetectionData detection_result;
            detection_result.camera_data = camera_data;
            detection_result.timestamp = xTaskGetTickCount();
            
            std::vector<tensorflow::Object> detections;
            
            if (detect_objects(&interpreter, camera_data, &detections)) {
                detection_result.detections = std::move(detections);
                detection_result.inference_time = xTaskGetTickCount() - detection_result.timestamp;

                if (xQueueOverwrite(g_detection_output_queue_m7, &detection_result) != pdTRUE) {
                    printf("ERROR: Failed to send detection result\r\n");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(33));
    }
}
}