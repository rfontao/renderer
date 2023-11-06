#pragma once

#include "VulkanDevice.h"

class VulkanShader {
public:
    VulkanShader(std::shared_ptr<VulkanDevice> device, const std::pair<std::string, std::string> &shaderPaths);

private:

    VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

    struct DescriptorSetLayoutData {
        VkDescriptorSetLayoutCreateInfo createInfo;
        std::map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
    };

    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

    std::shared_ptr<VulkanDevice> m_Device;
};

