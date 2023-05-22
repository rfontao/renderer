#pragma once

#include "VulkanDevice.h"

class VulkanPipeline {
public:

    struct DescriptorSetLayoutData {
        uint32_t setNumber;
        VkDescriptorSetLayoutCreateInfo createInfo;
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        bool operator==(const DescriptorSetLayoutData& other) const { return setNumber == other.setNumber; }
    };

    VulkanPipeline() = default;
    VulkanPipeline(std::shared_ptr<VulkanDevice> device, VkFormat colorAttachmentFormat);
    void Destroy();

    [[nodiscard]] VkPipeline GetPipeline() const { return m_Pipeline; }
    [[nodiscard]] VkPipelineLayout GetLayout() const { return m_Layout; }
    [[nodiscard]] const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }

private:
    [[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;

    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts; // index are sets

    std::shared_ptr<VulkanDevice> m_Device;
};
