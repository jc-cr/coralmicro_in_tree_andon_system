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

    // Send red, blue, green colors every second
    struct pattern {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };

    pattern all_red = {255, 0, 0};
    pattern all_green = {0, 255, 0};
    pattern all_blue = {0, 0, 255};


    // Store patterns for loop
    pattern patterns[3] = {all_red, all_green, all_blue};
    
    while (true) {
        for (int i = 0; i < 3; i++) {
            SendColor(patterns[i].red, patterns[i].green, patterns[i].blue);
            SendColor(patterns[i].red, patterns[i].green, patterns[i].blue);
            SendColor(patterns[i].red, patterns[i].green, patterns[i].blue);
            ResetDelay();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

} // namespace coralmicro