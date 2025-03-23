#include "pch.h"

#include "StagingManager.h"

void StagingManager::InitializeStagingBuffers(std::shared_ptr<VulkanDevice> device) {
    this->device = device;

    for (auto &stagingBuffer: stagingBuffers) {
        // 64 MB staging buffer
        stagingBuffer = {std::make_unique<VulkanBuffer>(
                device, 64 * 1024 * 1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT),
                0};
    }
}
void StagingManager::AddCopy(void* src, VkBuffer dstBuffer, VkDeviceSize size) {
    // TODO: Check bounds
    queuedBufferCopies.push_back({
            .destination = dstBuffer,
            .offset = stagingBuffers[currentFrame].currentOffset,
            .size = size,
    });

    // Copy data to staging buffer
    stagingBuffers[currentFrame].buffer->From(src, size, stagingBuffers[currentFrame].currentOffset);
    stagingBuffers[currentFrame].currentOffset += size;
}

void StagingManager::NextFrame() {
    currentFrame = (currentFrame + 1) % 2;
    stagingBuffers[currentFrame].currentOffset = 0;
    queuedBufferCopies.clear();
}

void StagingManager::Destroy() {
    for (const auto &stagingBuffer: stagingBuffers) {
        stagingBuffer.buffer->Destroy();
    }
}
