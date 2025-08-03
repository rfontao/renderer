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

//TODO: Duplicate vertex buffer on GPU????
void StagingManager::AddCopy(void *src, VkBuffer dstBuffer, VkDeviceSize size) {
    assert(stagingBuffers[currentFrame].currentOffset + size <= stagingBuffers[currentFrame].buffer->GetSize());

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

void StagingManager::Flush(VkCommandBuffer commandBuffer) {
    if (queuedBufferCopies.empty()) {
        return;
    }

    for (const auto &copy: queuedBufferCopies) {
        VkBufferCopy copyInfo{
                .srcOffset = copy.offset,
                .dstOffset = 0,
                .size = copy.size,
        };
        vkCmdCopyBuffer(commandBuffer, stagingBuffers[currentFrame].buffer->GetBuffer(), copy.destination, 1,
                        &copyInfo);
    }

    // TODO(RF): Check if this is correct
    VkMemoryBarrier2 memoryBarrier{.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                                   .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                   .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                   .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                   .dstAccessMask = VK_ACCESS_2_NONE};

    VkDependencyInfo dependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .memoryBarrierCount = 1,
            .pMemoryBarriers = &memoryBarrier,
    };

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

    // NOTE: Clear the queued copies after flushing but don't reset the current offset because the staging buffer is still
    //       needed for when the vkCmdCopyBuffer is actually executed ;-;
    queuedBufferCopies.clear();
}

void StagingManager::NextFrame() {
    currentFrame = (currentFrame + 1) % stagingBuffers.size();
    stagingBuffers[currentFrame].currentOffset = 0;
    queuedBufferCopies.clear();
}

void StagingManager::Destroy() {
    for (const auto &stagingBuffer: stagingBuffers) {
        stagingBuffer.buffer->Destroy();
    }
}
