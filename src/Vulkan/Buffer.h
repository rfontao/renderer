#pragma once

#include "VulkanDevice.h"

enum class BufferType {
    STAGING,
    VERTEX,
    INDEX,
    GPU,
};

struct BufferSpecification {
    std::string name;
    VkDeviceSize size{0};
    BufferType type{BufferType::GPU};
};

class VulkanDevice;

class Buffer {
public:
    Buffer() = delete;
    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    Buffer(std::shared_ptr<VulkanDevice> &device, BufferSpecification specification);
    void Destroy();

    void Map();
    void Unmap();

    // TODO: Make name more clear
    void From(void *src, VkDeviceSize srcSize);
    void From(void *src, VkDeviceSize srcSize, uint32_t offset);
    void Fill(uint8_t data, VkDeviceSize srcSize);
    void FromBuffer(Buffer *src);

    [[nodiscard]] VkBuffer GetBuffer() const { return buffer; }
    [[nodiscard]] VkDeviceAddress GetAddress() const { return address; }
    [[nodiscard]] BufferType GetType() const { return specification.type; }
    [[nodiscard]] size_t GetSize() const { return specification.size; }

private:
    VkBuffer buffer{VK_NULL_HANDLE};
    VmaAllocation m_Allocation;
    VmaAllocationInfo m_AllocationInfo;

    VkDeviceAddress address{0};

    bool isMapped = false;

    BufferSpecification specification;

    std::shared_ptr<VulkanDevice> m_Device;
};
