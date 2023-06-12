#pragma once

#include "VulkanDevice.h"

class VulkanDevice;

class VulkanBuffer {
public:
    VulkanBuffer() = default;
    VulkanBuffer(std::shared_ptr<VulkanDevice> device, VkDeviceSize size, VkBufferUsageFlags usage,
                 VkMemoryPropertyFlags properties);
    void Destroy();

    void Map();
    void Unmap();

    // TODO: Make name more clear
    void From(void *src, VkDeviceSize srcSize);
    void From(void *src, VkDeviceSize srcSize, uint32_t offset);
    void FromBuffer(VulkanBuffer& src);

    [[nodiscard]] VkBuffer GetBuffer() const { return m_Buffer; };

private:
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkDeviceSize m_Size = VK_NULL_HANDLE;
    VkBufferUsageFlags m_Usage = VK_NULL_HANDLE;
    void *m_Data = nullptr; // Must be mapped

    std::shared_ptr<VulkanDevice> m_Device;

};
