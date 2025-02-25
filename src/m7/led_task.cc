// led_task.cc
#include "m7/led_task.hh"

namespace coralmicro {

    inline void SendBit(bool bit) {
        _ZN10coralmicro7SendBitEb(bit);
    }

    inline void InitializeGpio() {
        _ZN10coralmicro15InitializeGpioEv();
    }

    inline void ResetDelay() {
        _ZN10coralmicro10ResetDelayEv();
    }

    inline void SendByte(uint8_t byte) {
        for (int i = 7; i >= 0; --i) {
            SendBit(byte & (1 << i));
        }
    }

    inline void SendColor(uint8_t red, uint8_t green, uint8_t blue) {
        uint32_t primask = DisableGlobalIRQ();
        SendByte(green);
        SendByte(red);
        SendByte(blue);
        EnableGlobalIRQ(primask);
    }

    void led_task(void* parameters) {
        (void)parameters;
        
        printf("LED task starting...\r\n");
        
        InitializeGpio();
        
        // Initialize current state to UNINITIALIZED
        SystemState current_state = SystemState::UNINITIALIZED;
        SystemState new_state;
        
        // Set initial color to WHITE (UNINITIALIZED)
        ResetDelay();
        for (int i = 0; i < 3; i++) {
            SendColor(255, 255, 255);
        }
        
        while (true) {
            // Wait for state updates from the queue with a timeout
            if (xQueueReceive(g_state_update_queue_m7, &new_state, pdMS_TO_TICKS(100)) == pdTRUE) {
                // Reset between updates for reliable communication
                ResetDelay();
                
                // Send same color to all three LEDs
                switch (new_state) {
                    case SystemState::UNINITIALIZED:
                        // WHITE for UNINITIALIZED
                        for (int i = 0; i < 3; i++) {
                            SendColor(255, 255, 255);
                        }
                        break;
                        
                    case SystemState::IDLE:
                        // BLUE for IDLE
                        for (int i = 0; i < 3; i++) {
                            SendColor(0, 0, 255);
                        }
                        break;
                        
                    case SystemState::WARNING:
                        // YELLOW for WARNING
                        for (int i = 0; i < 3; i++) {
                            SendColor(255, 255, 0);
                        }
                        break;
                        
                    case SystemState::ERROR:
                        // RED for ERROR
                        for (int i = 0; i < 3; i++) {
                            SendColor(255, 0, 0);
                        }
                        break;
                        
                    default:
                        // Default to white if unknown state
                        for (int i = 0; i < 3; i++) {
                            SendColor(255, 255, 255);
                        }
                        break;
                }
                
                // Update current state after change
                current_state = new_state;
                printf("LED color updated for state: %s\r\n", 
                    current_state == SystemState::IDLE ? "IDLE" :
                    current_state == SystemState::WARNING ? "WARNING" :
                    current_state == SystemState::ERROR ? "ERROR" : "UNINITIALIZED");
            }
            
            // Short delay to avoid busy-waiting
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
