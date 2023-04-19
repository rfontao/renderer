#pragma once

#include <string>
#include <memory>

#include "vulkan/vulkan.h"

#include "image.h"

class Texture {
public:
    Texture() = default;
    Texture(std::shared_ptr<Device> device, const std::string &path);
    void Destroy();

    [[nodiscard]] uint32_t GetWidth() const { return m_Image->GetWidth(); }
    [[nodiscard]] uint32_t GetHeight() const { return m_Image->GetHeight(); }
    [[nodiscard]] VkSampler GetSampler() const { return m_Sampler; }
    [[nodiscard]] std::shared_ptr<Image> GetImage() const { return m_Image; }

private:
    void LoadFromFile(const std::string &path);
    void CreateSampler();

    uint32_t m_MipLevelCount;
    uint32_t m_LayerCount;

    std::shared_ptr<Image> m_Image;
    VkSampler m_Sampler;

    std::shared_ptr<Device> m_Device;
};

