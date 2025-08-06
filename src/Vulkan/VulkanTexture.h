#pragma once

#include <string>
#include <memory>

#include "VulkanImage.h"

enum class TextureWrapMode {
    Clamp = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    Repeat = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    MirroredRepeat = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT
};

enum class TextureFilterMode {
    Linear = VK_FILTER_LINEAR,
    Nearest = VK_FILTER_NEAREST
};

struct TextureSampler {
    TextureWrapMode samplerWrap{TextureWrapMode::Repeat};
    TextureFilterMode samplerFilter{TextureFilterMode::Linear};
};

struct TextureSpecification {
    std::string name;
    ImageFormat format{ImageFormat::R8G8B8A8};
    uint32_t width{1};
    uint32_t height{1};
    TextureWrapMode samplerWrap{TextureWrapMode::Repeat};
    TextureFilterMode samplerFilter{TextureFilterMode::Linear};

    bool generateMipMaps{false};
};

class VulkanTexture {
public:
    VulkanTexture() = default;
    virtual ~VulkanTexture() = default;
    void Destroy();

    [[nodiscard]] VkSampler GetSampler() const { return m_Sampler; }
    [[nodiscard]] std::shared_ptr<VulkanImage> GetImage() const { return m_Image; }

    void SetSampler(const TextureSampler& sampler);

    std::shared_ptr<VulkanImage> m_Image;
    std::shared_ptr<VulkanDevice> m_Device;
    VkSampler m_Sampler { VK_NULL_HANDLE };
};

class Texture2D : public VulkanTexture {
public:
    Texture2D(std::shared_ptr<VulkanDevice> device, TextureSpecification &specification,
              const std::filesystem::path &path);
    Texture2D(std::shared_ptr<VulkanDevice> device, TextureSpecification &specification, void *pixels);
    Texture2D(std::shared_ptr<VulkanDevice> device, TextureSpecification &specification);
};

class TextureCube : public VulkanTexture {
public:
    TextureCube(std::shared_ptr<VulkanDevice> device, TextureSpecification &specification,
                const std::vector<std::filesystem::path> &paths);
};


