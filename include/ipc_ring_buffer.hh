// IPC ring buffer for inter-core communication
#pragma once

#include "libs/base/ipc_message_buffer.h"
#include <array>
#include <atomicV

namespace coralmicro {

// Message types for different sensors
enum class SensorType : uint8_t {
    kCamera,
    kTof,
    kAck,
    // Add more sensor types as needed
};

// Header for each message in ring buffer
struct MessageHeader {
    SensorType type;
    uint32_t sequence;  // Sequence number for tracking
    uint32_t size;      // Size of the payload
    TickType_t timestamp;
} __attribute__((packed));

// Fixed size buffer for each sensor type
template<size_t BufferSize = 2048>
class SensorBuffer {
public:
    bool push(const void* data, size_t size) {
        if (size > BufferSize) return false;
        
        uint32_t current_head = head_.load(std::memory_order_relaxed);
        uint32_t next_head = (current_head + 1) % kSlots;
        
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false;  // Buffer full
        }
        
        // Copy data to buffer
        memcpy(buffers_[current_head].data(), data, size);
        sizes_[current_head] = size;
        
        head_.store(next_head, std::memory_order_release);
        return true;
    }
    
    bool pop(void* data, size_t& size) {
        uint32_t current_tail = tail_.load(std::memory_order_relaxed);
        
        if (current_tail == head_.load(std::memory_order_acquire)) {
            return false;  // Buffer empty
        }
        
        // Copy data from buffer
        size = sizes_[current_tail];
        memcpy(data, buffers_[current_tail].data(), size);
        
        tail_.store((current_tail + 1) % kSlots, std::memory_order_release);
        return true;
    }
    
    void clear() {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

private:
    static constexpr size_t kSlots = 4;  // Number of buffer slots
    std::array<std::array<uint8_t, BufferSize>, kSlots> buffers_;
    std::array<size_t, kSlots> sizes_;
    std::atomic<uint32_t> head_{0};
    std::atomic<uint32_t> tail_{0};
};

// IPC Message Manager for handling multiple sensor streams
class IpcMessageManager {
public:
    static constexpr size_t kCameraBufferSize = 4096;  // Adjust based on max image size
    static constexpr size_t kTofBufferSize = 512;      // Adjust based on ToF data size
    
    bool sendMessage(SensorType type, const void* data, size_t size) {
        MessageHeader header{
            .type = type,
            .sequence = next_sequence_++,
            .size = static_cast<uint32_t>(size),
            .timestamp = xTaskGetTickCount()
        };
        
        switch (type) {
            case SensorType::kCamera:
                return camera_buffer_.push(data, size);
            case SensorType::kTof:
                return tof_buffer_.push(data, size);
            default:
                return false;
        }
    }
    
    bool receiveMessage(SensorType type, void* data, size_t& size) {
        switch (type) {
            case SensorType::kCamera:
                return camera_buffer_.pop(data, size);
            case SensorType::kTof:
                return tof_buffer_.pop(data, size);
            default:
                return false;
        }
    }
    
    void clear() {
        camera_buffer_.clear();
        tof_buffer_.clear();
    }

private:
    SensorBuffer<kCameraBufferSize> camera_buffer_;
    SensorBuffer<kTofBufferSize> tof_buffer_;
    std::atomic<uint32_t> next_sequence_{0};
};

// Global instances for each core
inline IpcMessageManager g_m4_ipc_manager;
inline IpcMessageManager g_m7_ipc_manager;

} // namespace coralmicro