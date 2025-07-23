#pragma once

#include "VulkanDevice.h"

class VulkanPipeline {
public:

    enum class CullingMode {
        NONE,
        FRONT,
        BACK,
        FRONT_AND_BACK
    };

    struct PipelineSpecification {
        std::filesystem::path vertShaderPath;
        std::filesystem::path fragShaderPath;
        std::filesystem::path compShaderPath;
        CullingMode cullingMode{CullingMode::BACK};
        bool depthBiasEnable{false};
        bool blendEnable{true};
        bool enableDepthTesting{true};
        bool wireframe{false};
    };

    struct DescriptorSetLayoutData {
        uint32_t setNumber{};
        VkDescriptorSetLayoutCreateInfo createInfo;
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        bool operator==(const DescriptorSetLayoutData &other) const { return setNumber == other.setNumber; }
    };

    VulkanPipeline(std::shared_ptr<VulkanDevice> device, PipelineSpecification &pipelineSpecification);

    void Destroy();

    [[nodiscard]] VkPipeline GetPipeline() const { return m_Pipeline; }

    [[nodiscard]] VkPipelineLayout GetLayout() const { return m_Layout; }

    [[nodiscard]] const std::vector<VkDescriptorSetLayout> &
    GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }

private:
    [[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;

    PipelineSpecification spec;

    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts; // index are sets
    std::shared_ptr<VulkanDevice> m_Device;
};
