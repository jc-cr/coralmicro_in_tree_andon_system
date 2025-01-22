#include "m7/rgb_task.hh"

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

void rgb_task(void* parameters) {
    (void)parameters;
    
    printf("RGB task starting...\r\n");
    
    InitializeGpio();
    
    RuntimeConfig latest_runtime_config;
    
    while (true) {
        // Get latest runtime config
        if (read_runtime_config(latest_runtime_config)) {
            uint32_t color = latest_runtime_config.running_state_color;
            
            uint8_t red = (color >> 16) & 0xFF;
            uint8_t green = (color >> 8) & 0xFF;
            uint8_t blue = color & 0xFF;
            
            SendColor(red, green, blue);
            SendColor(red, green, blue);
            SendColor(red, green, blue);
            ResetDelay();
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

} // namespace coralmicro