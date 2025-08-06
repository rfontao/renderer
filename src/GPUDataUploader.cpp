#include "pch.h"

#include "GPUDataUploader.h"

void GPUDataUploader::InitializeStagingBuffers(std::shared_ptr<VulkanDevice> device) {
    this->device = device;

    for (size_t i = 0; i < stagingBuffers.size(); ++i) {
        // 64 MB staging buffer
        const auto bufferName = std::format("StagingBuffer_{}", i);
        stagingBuffers[i] = {.buffer =
                                     std::make_unique<Buffer>(device, BufferSpecification{.name = bufferName,
                                                                                          .size = 64 * 1024 * 1024,
                                                                                          .type = BufferType::STAGING}),
                             .currentOffset = 0};
    }
}

// TODO: Duplicate vertex buffer on GPU????
void GPUDataUploader::AddCopy(void *src, VkBuffer dstBuffer, VkDeviceSize size) {
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

void GPUDataUploader::Flush(VkCommandBuffer commandBuffer) {
    if (queuedBufferCopies.empty()) {
        return;
    }

    std::vector<VkBufferMemoryBarrier2> memoryBarriers;
    for (const auto &copy: queuedBufferCopies) {
        VkBufferCopy copyInfo{
                .srcOffset = copy.offset,
                .dstOffset = 0,
                .size = copy.size,
        };
        vkCmdCopyBuffer(commandBuffer, stagingBuffers[currentFrame].buffer->GetBuffer(), copy.destination, 1,
                        &copyInfo);

        memoryBarriers.push_back({
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
                .buffer = copy.destination,
                .size = VK_WHOLE_SIZE,
        });
    }

    const VkDependencyInfo dependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = static_cast<uint32_t>(memoryBarriers.size()),
            .pBufferMemoryBarriers = memoryBarriers.data(),
    };

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

    // NOTE: Clear the queued copies after flushing but don't reset the current offset because the staging buffer is
    // still needed for when the vkCmdCopyBuffer is actually executed ;-;
    queuedBufferCopies.clear();
}

void GPUDataUploader::NextFrame() {
    currentFrame = (currentFrame + 1) % stagingBuffers.size();
    stagingBuffers[currentFrame].currentOffset = 0;
    queuedBufferCopies.clear();
}

void GPUDataUploader::Destroy() {
    for (const auto &stagingBuffer: stagingBuffers) {
        stagingBuffer.buffer->Destroy();
    }
}
