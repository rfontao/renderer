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
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkDeviceSize m_Size = VK_NULL_HANDLE;
    VkBufferUsageFlags m_Usage = VK_NULL_HANDLE;
    void *m_Data = nullptr; // Must be mapped

    std::shared_ptr<Device> m_Device;
};
