#include "pch.h"
#include "VulkanTexture.h"

#include "Utils.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"


TextureCube::TextureCube(std::shared_ptr<VulkanDevice> device, TextureSpecification &specification,
                         const std::vector<std::filesystem::path> &paths) {
    assert(paths.size() == 6);
    m_Device = device;

    int width, height, channels;
    auto res = stbi_info(paths[0].string().c_str(), &width, &height, &channels);
    if (res < 0) {
        throw std::runtime_error("Failed to get file information!");
    }

    VkDeviceSize imageSize = width * height * 4 * 6;
    VulkanBuffer stagingBuffer = VulkanBuffer(m_Device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VMA_MEMORY_USAGE_AUTO,
                                              VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                              VMA_ALLOCATION_CREATE_MAPPED_BIT);

    uint32_t offset = 0;
    for (const auto &path: paths) { // Should be 6
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("Failed to load texture!");
        }

        VkDeviceSize faceSize = texWidth * texHeight * 4;
        stagingBuffer.From(pixels, (size_t) faceSize, offset);
        offset += faceSize;

        stbi_image_free(pixels);
    }

    // Note: Currently ignoring mipmap generation for cube maps
    ImageSpecification imageSpecification{
            .format = specification.format,
            .width = specification.width,
            .height = specification.height,
            .mipLevels = 1,
            .layers = 6,
    };
    m_Image = make_shared<VulkanImage>(m_Device, imageSpecification);

    m_Image->TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    m_Image->CopyBufferData(stagingBuffer, 6);
    m_Image->TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    stagingBuffer.Destroy();

    SetSampler({.samplerWrap = specification.samplerWrap, .samplerFilter = specification.samplerFilter});
}

Texture2D::Texture2D(std::shared_ptr<VulkanDevice> device, TextureSpecification &specification,
                     const std::filesystem::path &path) {
    m_Device = device;
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("Failed to load texture!");
    }

    uint32_t mipLevelCount = 1;
    if (specification.generateMipMaps) {
        mipLevelCount = (uint32_t) (std::floor(std::log2(std::max(texWidth, texHeight))) + 1);
    }

    VulkanBuffer stagingBuffer = VulkanBuffer(m_Device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VMA_MEMORY_USAGE_AUTO,
                                              VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                              VMA_ALLOCATION_CREATE_MAPPED_BIT);

    stagingBuffer.From(pixels, (size_t) imageSize);

    stbi_image_free(pixels);

    ImageSpecification imageSpecification{
            .format = specification.format,
            .width = specification.width,
            .height = specification.height,
            .mipLevels = mipLevelCount,
            .layers = 1,
    };
    m_Image = make_shared<VulkanImage>(m_Device, imageSpecification);

    m_Image->TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    m_Image->CopyBufferData(stagingBuffer);
    if (specification.generateMipMaps) {
        m_Image->GenerateMipMaps(static_cast<VkFormat>(specification.format), mipLevelCount);
    }
    stagingBuffer.Destroy();

    SetSampler({.samplerWrap = specification.samplerWrap, .samplerFilter = specification.samplerFilter});
}

Texture2D::Texture2D(std::shared_ptr<VulkanDevice> device, TextureSpecification &specification) {
    m_Device = device;

    uint32_t mipLevelCount = 1;
    if (specification.generateMipMaps) {
        mipLevelCount = (uint32_t) (std::floor(std::log2(std::max(specification.width, specification.height))) + 1);
    }

    // TODO: Review usage later
    ImageSpecification imageSpecification{
            .format = specification.format,
            .usage = ImageUsage::Attachment,
            .width = specification.width,
            .height = specification.height,
            .mipLevels = mipLevelCount,
            .layers = 1,
    };
    m_Image = make_shared<VulkanImage>(m_Device, imageSpecification);

    if (specification.generateMipMaps) {
        m_Image->GenerateMipMaps(static_cast<VkFormat>(specification.format), mipLevelCount);
    }

    SetSampler({.samplerWrap = specification.samplerWrap, .samplerFilter = specification.samplerFilter});
}

Texture2D::Texture2D(std::shared_ptr<VulkanDevice> device, TextureSpecification &specification, void *pixels) {
    m_Device = device;

    int channels = GetChannels(specification.format);
    VkDeviceSize imageSize = specification.width * specification.height * channels;
    VulkanBuffer stagingBuffer = VulkanBuffer(m_Device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VMA_MEMORY_USAGE_AUTO,
                                              VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                              VMA_ALLOCATION_CREATE_MAPPED_BIT);

    stagingBuffer.From(pixels, imageSize);

    ImageSpecification imageSpecification{
            .format = specification.format,
            .width = specification.width,
            .height = specification.height,
            .mipLevels = 1,
            .layers = 1,
    };
    m_Image = make_shared<VulkanImage>(m_Device, imageSpecification);

    m_Image->TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    m_Image->CopyBufferData(stagingBuffer);
    m_Image->TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    stagingBuffer.Destroy();

    SetSampler({.samplerWrap = specification.samplerWrap, .samplerFilter = specification.samplerFilter});
}

void VulkanTexture::Destroy() {
    vkDestroySampler(m_Device->GetDevice(), m_Sampler, nullptr);
    m_Image->Destroy();
}

void VulkanTexture::SetSampler(const TextureSampler &sampler) {
    if (m_Sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_Device->GetDevice(), m_Sampler, nullptr);
    }

    // Create Sampler
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_Device->GetPhysicalDevice(), &properties);

    VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = static_cast<VkFilter>(sampler.samplerFilter),
            .minFilter = static_cast<VkFilter>(sampler.samplerFilter),
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = static_cast<VkSamplerAddressMode>(sampler.samplerWrap),
            .addressModeV = static_cast<VkSamplerAddressMode>(sampler.samplerWrap),
            .addressModeW = static_cast<VkSamplerAddressMode>(sampler.samplerWrap),
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f, // (float) mipLevels / 2, <- Force enable low mip maps
            .maxLod = (float) 10.0f, //TODO
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
    };

    VK_CHECK(vkCreateSampler(m_Device->GetDevice(), &samplerInfo, nullptr, &m_Sampler),
             "Failed to create texture sampler!");
}
