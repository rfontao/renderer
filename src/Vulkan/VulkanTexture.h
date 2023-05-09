#pragma once

#include <string>
#include <memory>

#include "vulkan/vulkan.h"

#include "VulkanImage.h"

class VulkanTexture {
public:
    VulkanTexture() = default;
    explicit VulkanTexture(std::shared_ptr<VulkanDevice> device);
    VulkanTexture(std::shared_ptr<VulkanDevice> device, const std::filesystem::path &path);
    VulkanTexture(std::shared_ptr<VulkanDevice> device, void *pixels, VkDeviceSize imageSize, int texWidth,
                  int texHeight);


    void Destroy();

    [[nodiscard]] uint32_t GetWidth() const { return m_Image->GetWidth(); }
    [[nodiscard]] uint32_t GetHeight() const { return m_Image->GetHeight(); }
    [[nodiscard]] VkSampler GetSampler() const { return m_Sampler; }
    [[nodiscard]] std::shared_ptr<VulkanImage> GetImage() const { return m_Image; }

private:
    void LoadFromFile(const std::filesystem::path &path);
    void CreateSampler();

    uint32_t m_MipLevelCount;
    uint32_t m_LayerCount;

    std::shared_ptr<VulkanImage> m_Image;
    VkSampler m_Sampler;

    std::shared_ptr<VulkanDevice> m_Device;
};

