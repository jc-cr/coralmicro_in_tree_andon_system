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
    
    printf("RGB task starting...\r\n");
    
    InitializeGpio();


    SystemState current_state;
    SystemState new_state;


    while (true) {

            new_state = g_system_state.load();

            if (new_state != current_state) {
                switch (new_state) {
                    case SystemState::IDLE:
                        SendColor(0, 255, 0); // Green
                        SendColor(0, 255, 0); // Green
                        SendColor(0, 255, 0); // Green
                        break;
                    case SystemState::WARNING:
                        SendColor(255, 255, 0); // Yellow
                        SendColor(255, 255, 0); // Yellow
                        SendColor(255, 255, 0); // Yellow
                        break;
                    case SystemState::ERROR:
                        SendColor(255, 0, 0); // Red
                        SendColor(255, 0, 0); // Red
                        SendColor(255, 0, 0); // Red
                        break;
                }
                current_state = new_state;
            }

            vTaskDelay(pdMS_TO_TICKS(33));
        }
    }
}
