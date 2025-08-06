#pragma once

#include <array>
#include <vector>

#include "Vulkan/Buffer.h"

class GPUDataUploader {
public:

    struct StagingBuffer {
        std::unique_ptr<Buffer> buffer;
        VkDeviceSize currentOffset;
    };

    struct BufferCopy {
        VkBuffer destination;
        VkDeviceSize offset;
        VkDeviceSize size;
    };

    GPUDataUploader() = default;

    void InitializeStagingBuffers(std::shared_ptr<VulkanDevice> device);
    void Destroy();

    void NextFrame();

    void AddCopy(void* src, VkBuffer dstBuffer, VkDeviceSize size);

    template <typename T>
    void AddCopy(std::vector<T>& vector, VkBuffer dstBuffer) {
        AddCopy(vector.data(), dstBuffer, vector.size() * sizeof(T));
    }

    void Flush(VkCommandBuffer commandBuffer);

    // Queued buffer copies for the current frame
    std::vector<BufferCopy> queuedBufferCopies;
    // Staging buffers for each frame in flight
    std::array<StagingBuffer, 2> stagingBuffers;

    uint32_t currentFrame = 0;

    std::shared_ptr<VulkanDevice> device;
};
