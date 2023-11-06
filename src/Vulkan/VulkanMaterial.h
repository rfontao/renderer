#pragma once

#include "VulkanDevice.h"
#include "VulkanShader.h"

class VulkanMaterial {
public:

    VulkanMaterial() = default;
    VulkanMaterial(std::shared_ptr<VulkanDevice> device, std::shared_ptr<VulkanShader> shader, bool skybox = false);
    void Destroy();

    [[nodiscard]] VkPipeline GetPipeline() const { return m_Pipeline; }
    [[nodiscard]] VkPipelineLayout GetLayout() const { return m_Layout; }
private:

    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;

    std::shared_ptr<VulkanShader> m_Shader;

    std::shared_ptr<VulkanDevice> m_Device;
};
