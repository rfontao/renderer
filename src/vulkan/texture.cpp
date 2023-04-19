#include "pch.h"
#include "texture.h"

#include "utils.h"
#include "buffer.h"
#include "image.h"

Texture::Texture(std::shared_ptr<Device> device, const std::string &path) : m_Device(device) {
    LoadFromFile(path);
}

void Texture::LoadFromFile(const std::string &path) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    // TODO: Enable loading of textures with precalculated mipmaps (KTX file format)
    m_MipLevelCount = (uint32_t) (std::floor(std::log2(std::max(texWidth, texHeight))) + 1);

    if (!pixels) {
        throw std::runtime_error("Failed to load texture!");
    }

    Buffer stagingBuffer = Buffer(m_Device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    stagingBuffer.Map();
    stagingBuffer.From(pixels, (size_t) imageSize);
    stagingBuffer.Unmap();

    stbi_image_free(pixels);

    m_Image = make_shared<Image>(m_Device, texWidth, texHeight, m_MipLevelCount, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB,
                  VK_IMAGE_TILING_OPTIMAL,
                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    m_Image->TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    m_Image->CopyBufferData(stagingBuffer);
    m_Image->GenerateMipMaps(VK_FORMAT_R8G8B8A8_SRGB, m_MipLevelCount);
    CreateSampler();
}

void Texture::CreateSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_Device->GetPhysicalDevice(), &properties);

    VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f, // (float) mipLevels / 2, <- Force enable low mip maps
            .maxLod = (float) m_MipLevelCount,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
    };

    VK_CHECK(vkCreateSampler(m_Device->GetDevice(), &samplerInfo, nullptr, &m_Sampler),
             "Failed to create texture sampler!");
}
