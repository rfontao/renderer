#include "pch.h"

#include "Buffer.h"
#include "DebugMarkers.h"

Buffer::Buffer(const std::shared_ptr<VulkanDevice>& device, const BufferSpecification& specification) :
    specification(specification), device(device) {
    allocationInfo.pMappedData = nullptr;

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
    } else if (specification.type == BufferType::GPU_INDIRECT) {
        usageFlags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    const VkBufferCreateInfo bufferInfo{
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
    } else if (specification.type == BufferType::GPU_INDIRECT) {
        allocationFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    }

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = allocationFlags;
    vmaCreateBuffer(device->GetAllocator(), &bufferInfo, &allocInfo, &buffer, &allocation, &allocationInfo);

    DebugMarkers::BufferMarker(device, buffer, this->specification.name);

    if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        const VkBufferDeviceAddressInfo deviceAddressInfo{
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = buffer,
        };

        address = vkGetBufferDeviceAddress(device->GetDevice(), &deviceAddressInfo);
    }
}

void Buffer::Destroy() {
    if (isMapped) {
        Unmap();
    }
    vmaDestroyBuffer(device->GetAllocator(), buffer, allocation);
}

void Buffer::Map() {
    vmaMapMemory(device->GetAllocator(), allocation, &allocationInfo.pMappedData);
    isMapped = true;
}

void Buffer::Unmap() {
    vmaUnmapMemory(device->GetAllocator(), allocation);
    isMapped = false;
}

void Buffer::From(void *src, VkDeviceSize srcSize) {
    if (allocationInfo.pMappedData == nullptr)
        throw std::runtime_error("Tried to copy to unmapped buffer");
    memcpy(allocationInfo.pMappedData, src, srcSize);
}

void Buffer::From(void *src, VkDeviceSize srcSize, uint32_t offset) {
    if (allocationInfo.pMappedData == nullptr)
        throw std::runtime_error("Tried to copy to unmapped buffer");
    memcpy(static_cast<uint8_t *>(allocationInfo.pMappedData) + offset, src, srcSize);
}

// TODO: Maybe receive size??
void Buffer::FromBuffer(Buffer *src) {
    VkCommandBuffer commandBuffer = device->BeginSingleTimeCommands();

    VkBufferCopy copyRegion{
            .size = specification.size,
    };
    vkCmdCopyBuffer(commandBuffer, src->buffer, buffer, 1, &copyRegion);

    device->EndSingleTimeCommands(commandBuffer);
}

void Buffer::Fill(uint8_t data, VkDeviceSize srcSize) {
    if (allocationInfo.pMappedData == nullptr)
        throw std::runtime_error("Tried to copy to unmapped buffer");
    std::memset(allocationInfo.pMappedData, data, srcSize);
}
