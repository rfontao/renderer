#pragma once

#include <array>
#include <vector>

#include "Vulkan/VulkanBuffer.h"

class StagingManager {
public:

    struct StagingBuffer {
        std::unique_ptr<VulkanBuffer> buffer;
        VkDeviceSize currentOffset;
    };

    struct BufferCopy {
        VkBuffer destination;
        VkDeviceSize offset;
        VkDeviceSize size;
    };

    StagingManager() = default;

    void InitializeStagingBuffers(std::shared_ptr<VulkanDevice> device);
    void Destroy();

    void NextFrame();

    void AddCopy(void* src, VkBuffer dstBuffer, VkDeviceSize size);

    std::vector<BufferCopy> queuedBufferCopies;
    // Staging buffers for each frame in flight
    std::array<StagingBuffer, 2> stagingBuffers;

    // Queued buffer copies for the current frame

    uint32_t currentFrame = 0;

    std::shared_ptr<VulkanDevice> device;
};
