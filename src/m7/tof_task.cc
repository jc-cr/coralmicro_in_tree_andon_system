// tof_task.cc
#include "m7/tof_task.hh"

namespace coralmicro {

    bool print_data_sample_flag = false;


    void print_task_starting() {
        printf("TOF task starting...\r\n");
    }

    bool init_gpio() {

        printf("GPIO Power-on sequence starting...\r\n");
        
        // Configure LPn pin
        GpioSetMode(kLpnPin, GpioMode::kOutput);
        
        // Reset sequence
        GpioSet(kLpnPin, false);  // Assert reset
        vTaskDelay(pdMS_TO_TICKS(100));  // Increased delay
        GpioSet(kLpnPin, true);   // Release reset
        vTaskDelay(pdMS_TO_TICKS(100));  // Increased delay
        
        printf("GPIO initialization complete\r\n");
        return true;
    }

    const char* get_error_string(uint8_t status) {
        switch(status) {
            case VL53L8CX_STATUS_OK:
                return "No error";
            case VL53L8CX_STATUS_INVALID_PARAM:
                return "Invalid parameter";
            case VL53L8CX_STATUS_ERROR:
                return "Major error";
            case VL53L8CX_STATUS_TIMEOUT_ERROR:
                return "Timeout error";
            case VL53L8CX_STATUS_CORRUPTED_FRAME:
                return "Corrupted frame";
            case VL53L8CX_STATUS_LASER_SAFETY:
                return "Laser safety error";
            case VL53L8CX_STATUS_XTALK_FAILED:
                return "Cross-talk calibration failed";
            case VL53L8CX_STATUS_FW_CHECKSUM_FAIL:
                return "Firmware checksum error";
            case VL53L8CX_MCU_ERROR:
                return "MCU error";
            default:
                return "Unknown error";
        }
    }

    void print_sensor_error(const char* operation, uint8_t status) {

        printf("Error during %s: [%d] %s\r\n", 
            operation, 
            status, 
            get_error_string(status));
    }

    bool init_sensor(VL53L8CX_Configuration* dev) {
        uint8_t status;
        uint8_t isAlive = 0;
        
        // Add delay after power-on
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Check if sensor is alive
        status = vl53l8cx_is_alive(dev, &isAlive);
        if (status != VL53L8CX_STATUS_OK || !isAlive) {
            print_sensor_error("checking sensor alive", status);
            return false;
        }
        printf("Sensor is alive\r\n");
        
        // Initialize sensor
        status = vl53l8cx_init(dev);
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("sensor initialization", status);
            return false;
        }
        printf("Sensor initialized\r\n");
        
        
        status = vl53l8cx_set_resolution(dev, g_tof_resolution.load());
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("setting resolution", status);
            return false;
        }
        printf("Resolution set to 4x4\r\n");

        // Set ranging mode to continuous
        status = vl53l8cx_set_ranging_mode(dev, VL53L8CX_RANGING_MODE_CONTINUOUS);
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("setting ranging mode", status);
            return false;
        }
        printf("Ranging mode set to continuous\r\n");
        
        // Increase ranging frequency for better temporal resolution
        status = vl53l8cx_set_ranging_frequency_hz(dev, kRangingFrequency); // Max 60Hz for 4x4
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("setting ranging frequency", status);
            return false;
        }
        printf("Ranging frequency set to %i Hz", kRangingFrequency);

        // Set target order to closest first
        status = vl53l8cx_set_target_order(dev, VL53L8CX_TARGET_ORDER_CLOSEST);
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("setting target order", status);
            return false;
        }
        printf("Target order set to closest first\r\n");

        // Reduce sharpener to improve detection of distant objects
        status = vl53l8cx_set_sharpener_percent(dev, kSharpnerValue);
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("setting sharpener", status);
            return false;
        }
        printf("Sharpener set to %i%%\r\n", kSharpnerValue);

        return true;
    }

    void print_task_ok() {
        printf("TOF task started successfully\r\n");
    }

    void tof_task(void* parameters) {
        (void)parameters;
        
        print_task_starting();

        uint8_t status;
        
        
        // Start ranging
        status = vl53l8cx_start_ranging(g_tof_device.get());
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("starting ranging", status);
            vTaskSuspend(nullptr);
        }
        
        
        // Allocate results structure on heap
        auto results = std::make_unique<VL53L8CX_ResultsData>();
        if (!results) {
            printf("Failed to allocate results structure\r\n");
            vTaskSuspend(nullptr);
        }

        print_task_ok();
        

        int Hz = kRangingFrequency;
        TickType_t last_wake_time = xTaskGetTickCount();
        const TickType_t frequency = pdMS_TO_TICKS(1000 / Hz);

        while (true) {
            uint8_t isReady = 0;
            
            // Check if new data is ready
            status = vl53l8cx_check_data_ready(g_tof_device.get(), &isReady);
            
            if (status == VL53L8CX_STATUS_OK && isReady) {
                status = vl53l8cx_get_ranging_data(g_tof_device.get(), results.get());

                if (status == VL53L8CX_STATUS_OK) {
                    // Only print once to reduce console output load
                    if (!print_data_sample_flag) {
                        printf("\nTOF Grid (mm):\r\n");
                        printf("    C0    C1    C2    C3\r\n");
                        for(int row = 0; row < 4; row++) {
                            printf("R%d:", row);
                            for(int col = 0; col < 4; col++) {
                                int idx = row * 4 + col;
                                printf(" %4d", results->distance_mm[idx]);
                            }
                            printf("\r\n");
                        }
                        print_data_sample_flag = true;
                    }

                    // Send data to queue
                    if (xQueueOverwrite(g_tof_queue_m7, results.get()) != pdTRUE) {
                        printf("Failed to send TOF data to queue\r\n");
                    }
                } else {
                    print_sensor_error("getting ranging data", status);
                }
            } else if (status != VL53L8CX_STATUS_OK) {
                print_sensor_error("checking data ready", status);
            }
            
            // Use vTaskDelayUntil for more precise timing
            vTaskDelayUntil(&last_wake_time, frequency);
        }
    }
} // namespace coralmicro