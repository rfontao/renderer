#include "pch.h"
#include "VulkanBuffer.h"

#include "Utils.h"

VulkanBuffer::VulkanBuffer(std::shared_ptr<VulkanDevice> device, VkDeviceSize size, VkBufferUsageFlags usage,
                           VmaMemoryUsage vmaUsage, VmaAllocationCreateFlags vmaFlags) : m_Device(device), m_Size(size) {

    VkBufferCreateInfo bufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = vmaUsage;
    allocInfo.flags = vmaFlags;
    vmaCreateBuffer(m_Device->GetAllocator(), &bufferInfo, &allocInfo, &m_Buffer, &m_Allocation, &m_AllocationInfo);

    // if (vmaFlags & VMA_ALLOCATION_CREATE_MAPPED_BIT) {
    //     m_Mapped = true;
    // }
}

void VulkanBuffer::Destroy() {
    if (m_Mapped) {
        Unmap();
    }
    vmaDestroyBuffer(m_Device->GetAllocator(), m_Buffer, m_Allocation);
}

void VulkanBuffer::Map() {
    vmaMapMemory(m_Device->GetAllocator(), m_Allocation, &m_AllocationInfo.pMappedData);
    m_Mapped = true;
//    vkMapMemory(m_Device->GetDevice(), m_Memory, 0, m_Size, 0, &m_Data);
}

void VulkanBuffer::Unmap() {
    vmaUnmapMemory(m_Device->GetAllocator(), m_Allocation);
    m_Mapped = false;
//    vkUnmapMemory(m_Device->GetDevice(), m_Memory);
}

void VulkanBuffer::From(void *src, VkDeviceSize srcSize) {
    if (m_AllocationInfo.pMappedData == nullptr)
        throw std::runtime_error("Tried to copy to unmapped buffer");
    memcpy(m_AllocationInfo.pMappedData, src, srcSize);
}

void VulkanBuffer::From(void *src, VkDeviceSize srcSize, uint32_t offset) {
    if (m_AllocationInfo.pMappedData == nullptr)
        throw std::runtime_error("Tried to copy to unmapped buffer");
    memcpy(static_cast<uint8_t *>(m_AllocationInfo.pMappedData) + offset, src, srcSize);
}

// TODO: Maybe receive size??
void VulkanBuffer::FromBuffer(VulkanBuffer &src) {
    VkCommandBuffer commandBuffer = m_Device->BeginSingleTimeCommands();

    VkBufferCopy copyRegion{
            .size = m_Size,
    };
    vkCmdCopyBuffer(commandBuffer, src.m_Buffer, m_Buffer, 1, &copyRegion);

    m_Device->EndSingleTimeCommands(commandBuffer);
}

void VulkanBuffer::Fill(uint8_t data, VkDeviceSize srcSize) {
    if (m_AllocationInfo.pMappedData == nullptr)
        throw std::runtime_error("Tried to copy to unmapped buffer");
    std::memset(m_AllocationInfo.pMappedData, data, srcSize);
}
