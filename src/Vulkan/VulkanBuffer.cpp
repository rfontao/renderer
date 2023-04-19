#include "pch.h"
#include "VulkanBuffer.h"

#include "Utils.h"

VulkanBuffer::VulkanBuffer(std::shared_ptr<VulkanDevice> device, VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties) : m_Device(device), m_Size(size), m_Usage(usage) {

    VkBufferCreateInfo bufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = m_Size,
            .usage = m_Usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_CHECK(vkCreateBuffer(m_Device->GetDevice(), &bufferInfo, nullptr, &m_Buffer),
             "Failed to create staging buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_Device->GetDevice(), m_Buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = m_Device->FindMemoryType(memRequirements.memoryTypeBits, properties),
    };
    VK_CHECK(vkAllocateMemory(m_Device->GetDevice(), &allocInfo, nullptr, &m_Memory),
             "Failed to allocate vertex buffer memory!");

    vkBindBufferMemory(m_Device->GetDevice(), m_Buffer, m_Memory, 0);
}

void VulkanBuffer::Destroy() {
    vkDestroyBuffer(m_Device->GetDevice(), m_Buffer, nullptr);
    vkFreeMemory(m_Device->GetDevice(), m_Memory, nullptr);
}

void VulkanBuffer::Map() {
    vkMapMemory(m_Device->GetDevice(), m_Memory, 0, m_Size, 0, &m_Data);
}

void VulkanBuffer::Unmap() {
    vkUnmapMemory(m_Device->GetDevice(), m_Memory);
}

void VulkanBuffer::From(void *src, VkDeviceSize srcSize) {
    if (m_Data == nullptr)
        throw std::runtime_error("Tried to copy to unmapped buffer");
    memcpy(m_Data, src, srcSize);
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
