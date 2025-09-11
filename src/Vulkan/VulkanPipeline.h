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

    enum class DepthCompareOp {
        LESS = VK_COMPARE_OP_LESS,
        LESS_OR_EQUAL = VK_COMPARE_OP_LESS_OR_EQUAL,
        GREATER = VK_COMPARE_OP_GREATER,
        GREATER_OR_EQUAL = VK_COMPARE_OP_GREATER_OR_EQUAL,
        EQUAL = VK_COMPARE_OP_EQUAL,
        NOT_EQUAL = VK_COMPARE_OP_NOT_EQUAL,
        ALWAYS = VK_COMPARE_OP_ALWAYS,
        NEVER = VK_COMPARE_OP_NEVER
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
        DepthCompareOp depthCompareOp{DepthCompareOp::LESS};
        bool enableDepthWrite{true};
    };

    struct DescriptorSetLayoutData {
        uint32_t setNumber{};
        VkDescriptorSetLayoutCreateInfo createInfo;
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        bool operator==(const DescriptorSetLayoutData &other) const { return setNumber == other.setNumber; }
    };

    VulkanPipeline(std::shared_ptr<VulkanDevice> device, PipelineSpecification &pipelineSpecification);

    void Destroy();

    [[nodiscard]] VkPipeline GetPipeline() const { return pipeline; }

    [[nodiscard]] VkPipelineLayout GetLayout() const { return layout; }

    [[nodiscard]] const std::vector<VkDescriptorSetLayout> &
    GetDescriptorSetLayouts() const { return descriptorSetLayouts; }

private:
    [[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    PipelineSpecification spec;

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts; // index are sets
    std::shared_ptr<VulkanDevice> device;
};
