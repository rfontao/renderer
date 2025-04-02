#include "pch.h"

#include "StagingManager.h"

void StagingManager::InitializeStagingBuffers(std::shared_ptr<VulkanDevice> device) {
    this->device = device;

    for (auto &stagingBuffer: stagingBuffers) {
        // 64 MB staging buffer
        stagingBuffer = {std::make_unique<Buffer>(
                                 device, BufferSpecification{.size = 64 * 1024 * 1024, .type = BufferType::STAGING}),
                         0};
    }
}
void StagingManager::AddCopy(void *src, VkBuffer dstBuffer, VkDeviceSize size) {
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
