#include "m7/inference_task.hh"


namespace coralmicro {

    bool detect_objects(tflite::MicroInterpreter* interpreter, 
                    const CameraData& camera_data,
                    std::vector<tensorflow::Object>* results, const RuntimeConfig& runtime_config) {
        
        if (!results) return false;
        
        auto* input_tensor = interpreter->input_tensor(0);
        const int model_height = input_tensor->dims->data[1];
        const int model_width = input_tensor->dims->data[2];
        
        // Verify image dimensions match model input
        if (camera_data.width != model_width || camera_data.height != model_height) {
            printf("Image dimensions don't match model input requirements\r\n");
            return false;
        }
        
        // Copy image data to input tensor
        std::memcpy(tflite::GetTensorData<uint8_t>(input_tensor), 
                    camera_data.image_data->data(),
                    camera_data.image_data->size());
        
        // Run inference
        if (interpreter->Invoke() != kTfLiteOk) {
            printf("Inference failed\r\n");
            return false;
        }
        
        // Get all detection results with minimum score threshold
        auto all_detections = tensorflow::GetDetectionResults(interpreter,
            runtime_config.person_detection_min_confidence,
            10);
            // params: interpreter, threshold, max_results
        
        // Filter for person class (COCO class ID 0)
        results->clear();
        for (const auto& detection : all_detections) {
            if (detection.id == 0) {  // Person class
                results->push_back(detection);
                printf("Found person! Score: %.2f, Bbox: (%.2f, %.2f, %.2f, %.2f)\r\n",
                    detection.score,
                    detection.bbox.xmin, detection.bbox.ymin,
                    detection.bbox.xmax, detection.bbox.ymax);
            }
        }
        
        return true;
    }

    void inference_task(void* parameters) {
        (void)parameters;
        printf("Inference task starting...\r\n");

        // Init data structures
        RuntimeConfig latest_runtime_config = g_runtime_config; // Copy initial runtime config
        CameraData latest_camera_data;
        InferenceData latest_inference_data;
        
        // Load model
        std::vector<uint8_t> model;
        if (!LfsReadFile(kModelPath, &model)) {
            printf("Failed to load model from %s\r\n", kModelPath);
            vTaskSuspend(nullptr);
            return;
        }
        
        // Initialize EdgeTPU
        auto tpu_context = EdgeTpuManager::GetSingleton()->OpenDevice();
        if (!tpu_context) {
            printf("Failed to initialize EdgeTPU\r\n");
            vTaskSuspend(nullptr);
            return;
        }
        
        // Setup TFLite interpreter
        tflite::MicroErrorReporter error_reporter;
        tflite::MicroMutableOpResolver<3> resolver;
        resolver.AddDequantize();
        resolver.AddDetectionPostprocess();
        resolver.AddCustom(kCustomOp, RegisterCustomOp());
        
        tflite::MicroInterpreter interpreter(
            tflite::GetModel(model.data()),
            resolver,
            tensor_arena,
            kTensorArenaSize,
            &error_reporter
        );
        
        if (interpreter.AllocateTensors() != kTfLiteOk) {
            printf("Failed to allocate tensors\r\n");
            vTaskSuspend(nullptr);
            return;
        }
        
        // Main task loop
        while (true) {

            // Get latest runtime config
            read_runtime_config(latest_runtime_config);

            // Try to get latest camera frame
            if (xQueuePeek(*InferenceTaskQueues::input_queue, &latest_camera_data, 0) == pdTRUE) {
                latest_inference_data.camera_data = latest_camera_data;
                latest_inference_data.timestamp = xTaskGetTickCount();

                // Run object detection
                if (detect_objects(&interpreter, latest_camera_data, &latest_inference_data.results, latest_runtime_config)) {
                    // Print detection results for debugging
                    printf("Detection results:\r\n%s\r\n", 
                        tensorflow::FormatDetectionOutput(latest_inference_data.results).c_str());
                        
                    // Store model dimensions for reference
                    latest_inference_data.model_width = interpreter.input_tensor(0)->dims->data[2];
                    latest_inference_data.model_height = interpreter.input_tensor(0)->dims->data[1];

                    if (xQueueOverwrite(*InferenceTaskQueues::output_queue, &latest_inference_data) != pdTRUE) {
                        printf("Failed to send inference data to queue\r\n");
                    }
                    
                }
            }
            
            vTaskDelay(pdMS_TO_TICKS(33));  // ~30 FPS
        }
    }

} // namespace coralmicro