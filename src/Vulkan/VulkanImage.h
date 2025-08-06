#pragma once

#include "pch.h"
#include "VulkanDevice.h"
#include "Buffer.h"

class VulkanDevice;

class Buffer;

enum class ImageFormat {
    R8 = VK_FORMAT_R8_UNORM,
    R8G8 = VK_FORMAT_R8G8_UNORM,
    R8G8B8 = VK_FORMAT_R8G8B8_UNORM,
    R8G8B8A8 = VK_FORMAT_R8G8B8A8_UNORM,
    R8G8B8A8_SRGB = VK_FORMAT_B8G8R8A8_SRGB,
    D16 = VK_FORMAT_D16_UNORM,
    D32 = VK_FORMAT_D32_SFLOAT,
    D24S8 = VK_FORMAT_D32_SFLOAT_S8_UINT,
};

enum class ImageUsage {
    Texture = 0,
    Attachment,
};

struct ImageSpecification {
    std::string name;
    ImageFormat format{ImageFormat::R8G8B8A8};
    ImageUsage usage{ImageUsage::Texture};
    uint32_t width{1};
    uint32_t height{1};
    uint32_t mipLevels{1};
    uint32_t layers{1};
};

class VulkanImage {
public:
    VulkanImage() = default;
    VulkanImage(std::shared_ptr<VulkanDevice> device, const ImageSpecification &specification);

    // Swapchain Image ctor
    VulkanImage(std::shared_ptr<VulkanDevice> device, VkImage image);

    void Destroy();

    void TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
    void TransitionLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);

    void CopyBufferData(Buffer &buffer, uint32_t layerCount = 1);
    void GenerateMipMaps(VkFormat format, uint32_t mipLevelCount, bool cube = false);

    [[nodiscard]] uint32_t GetWidth() const { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const { return m_Height; }
    [[nodiscard]] VkImageView GetImageView() const { return m_View; }
    [[nodiscard]] VkImage GetImage() const { return m_Image; }

    bool m_IsSwapchainImage = false;
private:
    uint32_t m_Width{0}, m_Height{0};
    VkImage m_Image{VK_NULL_HANDLE};
    VkImageView m_View{VK_NULL_HANDLE};
    VkDeviceMemory m_Memory{VK_NULL_HANDLE};
    VkFormat m_Format;

    VmaAllocation m_Allocation;

    std::shared_ptr<VulkanDevice> m_Device;
};

[[nodiscard]] bool IsDepthFormat(ImageFormat format);
[[nodiscard]] int GetChannels(ImageFormat format);
