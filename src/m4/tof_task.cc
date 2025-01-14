// tof_task.cc
#include "m4/tof_task.hh"

namespace coralmicro {

    void print_results(VL53L8CX_ResultsData* results) {
        // Print header with temperature
        printf("\r\n=== VL53L8CX Sensor Reading (Temp: %d°C) ===\r\n\r\n", 
            results->silicon_temp_degc);
        
        // Print column headers
        printf("     ");
        for(int col = 0; col < 8; col++) {
            printf("  C%d   ", col);
        }
        printf("\r\n");
        
        // Print separator
        printf("     ");
        for(int col = 0; col < 8; col++) {
            printf("------");
        }
        printf("\r\n");
        
        // Print each row
        for(int row = 0; row < 8; row++) {
            printf("R%d | ", row);
            for(int col = 0; col < 8; col++) {
                int zone = row * 8 + col;
                
                if(results->nb_target_detected[zone] == 0) {
                    printf(" ---- ");
                } else {
                    // Only show distance in mm, padded to 4 digits
                    printf("%5d ", results->distance_mm[zone]);
                }
            }
            printf("|\r\n");
        }
        
        // Print separator
        printf("     ");
        for(int col = 0; col < 8; col++) {
            printf("------");
        }
        printf("\r\n\r\n");
        
        // Print statistics for valid measurements only
        printf("Valid measurements (Status=5):\r\n");
        for(uint8_t i = 0; i < kResolution; i++) {
            if(results->nb_target_detected[i] > 0 && 
            results->target_status[i] == 5) {
                printf("Zone %2d: %4dmm (Signal: %4d)\r\n", 
                    i, 
                    results->distance_mm[i],
                    results->signal_per_spad[i]);
            }
        }
        printf("\r\n");  // Extra line for spacing between updates
        
        fflush(stdout);
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
        fflush(stdout);
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
        
        // Add delay before initialization
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Initialize sensor
        status = vl53l8cx_init(dev);
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("sensor initialization", status);
            return false;
        }
        printf("Sensor initialized\r\n");
        
        // Add delay after initialization
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Set resolution
        status = vl53l8cx_set_resolution(dev, kResolution);
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("setting resolution", status);
            return false;
        }
        printf("Resolution set to 8x8\r\n");
        
        // Add delay between configuration steps
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Set ranging mode to continuous
        status = vl53l8cx_set_ranging_mode(dev, VL53L8CX_RANGING_MODE_CONTINUOUS);
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("setting ranging mode", status);
            return false;
        }
        printf("Ranging mode set to continuous\r\n");
        
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Set ranging frequency
        status = vl53l8cx_set_ranging_frequency_hz(dev, kRangingFrequency);
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("setting ranging frequency", status);
            return false;
        }
        printf("Ranging frequency set to %d Hz\r\n", kRangingFrequency);
        
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Set integration time
        status = vl53l8cx_set_integration_time_ms(dev, kIntegrationTime);
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("setting integration time", status);
            return false;
        }
        printf("Integration time set to %d ms\r\n", kIntegrationTime);
        
        // Final delay before returning
        vTaskDelay(pdMS_TO_TICKS(50));
        
        return true;
    }

    void tof_task(void* parameters) {
        (void)parameters;
        uint8_t status;
        
        printf("TOF task starting...\r\n");
        fflush(stdout);
        
        if (!init_gpio()) {
            printf("GPIO initialization failed\r\n");
            return;
        }
        
        // Platform initialization with proper cleanup
        VL53L8CX_Platform platform = {};
        if (!vl53l8cx::PlatformInit(&platform, kI2c, kAddress)) {
            printf("Platform initialization failed\r\n");
            return;
        }
        
        // Create and initialize device instance
        auto dev = std::make_unique<VL53L8CX_Configuration>();
        if (!dev) {
            printf("Failed to allocate device configuration\r\n");
            return;
        }
        
        dev->platform = platform;
        
        
        if (!init_sensor(dev.get())) {
            printf("Sensor initialization failed - exiting task\r\n");
            return;
        }
        
        // Start ranging
        status = vl53l8cx_start_ranging(dev.get());
        if (status != VL53L8CX_STATUS_OK) {
            print_sensor_error("starting ranging", status);
            return;
        }
        
        printf("Ranging started successfully\r\n");
        fflush(stdout);
        
        // Allocate results structure on heap
        auto results = std::make_unique<VL53L8CX_ResultsData>();
        if (!results) {
            printf("Failed to allocate results structure\r\n");
            return;
        }
        
        // Main task loop with watchdog
        TickType_t last_wake_time = xTaskGetTickCount();
        const TickType_t frequency = pdMS_TO_TICKS(33);
        
        while (true) {
            uint8_t isReady = 0;
            
            // Check if new data is ready
            status = vl53l8cx_check_data_ready(dev.get(), &isReady);
            
            if (status == VL53L8CX_STATUS_OK && isReady) {
                status = vl53l8cx_get_ranging_data(dev.get(), results.get());
                if (status == VL53L8CX_STATUS_OK) {
                    xQueueOverwrite(TofTaskQueues::output_queue, results.get());  // Always keep latest frame
                } else {
                    print_sensor_error("getting ranging data", status);
                }
            } else if (status != VL53L8CX_STATUS_OK) {
                print_sensor_error("checking data ready", status);
            }
            
            // Use vTaskDelayUntil for consistent timing
            vTaskDelayUntil(&last_wake_time, frequency);
        }
    }
} // namespace coralmicro
