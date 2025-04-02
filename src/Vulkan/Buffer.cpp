#include "Buffer.h"
#include "pch.h"

Buffer::Buffer(std::shared_ptr<VulkanDevice>& device, BufferSpecification specification) :
    specification(std::move(specification)), m_Device(device) {
    m_AllocationInfo.pMappedData = nullptr;

    VkBufferUsageFlags usageFlags = 0;
    if (specification.type == BufferType::STAGING) {
        usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    } else if (specification.type == BufferType::VERTEX) {
        usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    } else if (specification.type == BufferType::INDEX) {
        usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    } else if (specification.type == BufferType::GPU) {
        usageFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    VkBufferCreateInfo bufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = specification.size,
            .usage = usageFlags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateFlags allocationFlags = 0;
    if (specification.type == BufferType::STAGING) {
        allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else if (specification.type == BufferType::VERTEX) {
        allocationFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    } else if (specification.type == BufferType::INDEX) {
        allocationFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    } else if (specification.type == BufferType::GPU) {
        allocationFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    }

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = allocationFlags;
    vmaCreateBuffer(m_Device->GetAllocator(), &bufferInfo, &allocInfo, &buffer, &m_Allocation, &m_AllocationInfo);

    if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        const VkBufferDeviceAddressInfo deviceAddressInfo{
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = buffer,
        };

        address = vkGetBufferDeviceAddress(m_Device->GetDevice(), &deviceAddressInfo);
    }
}

void Buffer::Destroy() {
    if (isMapped) {
        Unmap();
    }
    vmaDestroyBuffer(m_Device->GetAllocator(), buffer, m_Allocation);
}

void Buffer::Map() {
    vmaMapMemory(m_Device->GetAllocator(), m_Allocation, &m_AllocationInfo.pMappedData);
    isMapped = true;
}

void Buffer::Unmap() {
    vmaUnmapMemory(m_Device->GetAllocator(), m_Allocation);
    isMapped = false;
}

void Buffer::From(void *src, VkDeviceSize srcSize) {
    if (m_AllocationInfo.pMappedData == nullptr)
        throw std::runtime_error("Tried to copy to unmapped buffer");
    memcpy(m_AllocationInfo.pMappedData, src, srcSize);
}

void Buffer::From(void *src, VkDeviceSize srcSize, uint32_t offset) {
    if (m_AllocationInfo.pMappedData == nullptr)
        throw std::runtime_error("Tried to copy to unmapped buffer");
    memcpy(static_cast<uint8_t *>(m_AllocationInfo.pMappedData) + offset, src, srcSize);
}

// TODO: Maybe receive size??
void Buffer::FromBuffer(Buffer *src) {
    VkCommandBuffer commandBuffer = m_Device->BeginSingleTimeCommands();

    VkBufferCopy copyRegion{
            .size = specification.size,
    };
    vkCmdCopyBuffer(commandBuffer, src->buffer, buffer, 1, &copyRegion);

    m_Device->EndSingleTimeCommands(commandBuffer);
}

void Buffer::Fill(uint8_t data, VkDeviceSize srcSize) {
    if (m_AllocationInfo.pMappedData == nullptr)
        throw std::runtime_error("Tried to copy to unmapped buffer");
    std::memset(m_AllocationInfo.pMappedData, data, srcSize);
}
