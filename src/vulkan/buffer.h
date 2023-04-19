#pragma once

#include "device.h"

class Device;

class Buffer {
public:
    Buffer() = default;
    Buffer(std::shared_ptr<Device> device, VkDeviceSize size, VkBufferUsageFlags usage,
           VkMemoryPropertyFlags properties);
    ~Buffer();

    void Map();
    void Unmap();

    // TODO: Make name more clear
    void From(void *src, VkDeviceSize srcSize);
    void FromBuffer(Buffer& src);

    [[nodiscard]] VkBuffer GetBuffer() const { return m_Buffer; };

private:
    VkBuffer m_Buffer;
    VkDeviceMemory m_Memory;
    VkDeviceSize m_Size;
    VkBufferUsageFlags m_Usage;
    void *m_Data = nullptr; // Must be mapped

    std::shared_ptr<Device> m_Device;
};
