#pragma once

#include "device.h"
#include "buffer.h"

class Device;
class Buffer;

class Image {
public:
    Image() = default;
    Image(std::shared_ptr<Device> device, uint32_t width, uint32_t height, uint32_t mipLevelCount,
          VkSampleCountFlagBits numSamples, VkFormat format,
          VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
          VkImageAspectFlags aspectFlags);

    explicit Image(std::shared_ptr<Device> device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
                   uint32_t mipLevels);
    void Destroy();

    void TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
    void CopyBufferData(Buffer &buffer);
    void GenerateMipMaps(VkFormat format, uint32_t mipLevelCount);

    [[nodiscard]] uint32_t GetWidth() const { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const { return m_Height; }
    [[nodiscard]] VkImageView GetImageView() const { return m_View; }
    [[nodiscard]] VkImage GetImage() const { return m_Image; }

    bool m_IsSwapchainImage = false;
private:
    uint32_t m_Width, m_Height;
    VkImage m_Image = VK_NULL_HANDLE;
    VkImageView m_View = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;

    std::shared_ptr<Device> m_Device;
};