#pragma once

#include "VulkanDevice.h"

class VulkanPipeline {
public:

    struct DescriptorSetLayoutData {
        uint32_t setNumber;
        VkDescriptorSetLayoutCreateInfo createInfo;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
    };

    VulkanPipeline() = default;
    VulkanPipeline(std::shared_ptr<VulkanDevice> device, VkFormat colorAttachmentFormat);
    void Destroy();

    [[nodiscard]] VkPipeline GetPipeline() const { return m_Pipeline; }
    [[nodiscard]] VkPipelineLayout GetLayout() const { return m_Layout; }
    [[nodiscard]] VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

private:
    [[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;

    std::shared_ptr<VulkanDevice> m_Device;
};
