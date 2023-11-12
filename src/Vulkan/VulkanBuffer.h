#pragma once

#include "pch.h"
#include "VulkanDevice.h"

class VulkanDevice;

class VulkanBuffer {
public:
    VulkanBuffer() = default;
    VulkanBuffer(std::shared_ptr<VulkanDevice> device, VkDeviceSize size, VkBufferUsageFlags usage,
                 VmaMemoryUsage vmaUsage, VmaAllocationCreateFlags vmaFlags = 0);
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
    VkDeviceSize m_Size;
    VmaAllocation m_Allocation;
    VmaAllocationInfo m_AllocationInfo;
//    void *m_Data = nullptr; // Must be mapped

    bool m_Mapped = false;

    std::shared_ptr<VulkanDevice> m_Device;

};
