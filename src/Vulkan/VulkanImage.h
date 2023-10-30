#pragma once

#include "../pch.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"

class VulkanDevice;
class VulkanBuffer;

class VulkanImage {
public:
    VulkanImage() = default;
    VulkanImage(std::shared_ptr<VulkanDevice> device, uint32_t width, uint32_t height, uint32_t mipLevelCount,
                VkSampleCountFlagBits numSamples, VkFormat format,
                VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                VkImageAspectFlags aspectFlags);

    explicit VulkanImage(std::shared_ptr<VulkanDevice> device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
                         uint32_t mipLevels);

    VulkanImage(std::shared_ptr<VulkanDevice> device, uint32_t width, uint32_t height, uint32_t mipLevelCount,
                VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags, bool cube);

    void Destroy();

    void TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
    void CopyBufferData(VulkanBuffer &buffer, uint32_t layerCount = 1);
    void GenerateMipMaps(VkFormat format, uint32_t mipLevelCount, bool cube = false);

    [[nodiscard]] uint32_t GetWidth() const { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const { return m_Height; }
    [[nodiscard]] VkImageView GetImageView() const { return m_View; }
    [[nodiscard]] VkImage GetImage() const { return m_Image; }

    bool m_IsSwapchainImage = false;
private:
    uint32_t m_Width, m_Height;
    VkImage m_Image             { VK_NULL_HANDLE };
    VkImageView m_View          { VK_NULL_HANDLE };
    VkDeviceMemory m_Memory     { VK_NULL_HANDLE };
    VkFormat m_Format;

    VmaAllocation m_Allocation;

    std::shared_ptr<VulkanDevice> m_Device;
};