#include "pch.h"
#include <spirv_reflect.h>

#include "VulkanShader.h"

VulkanShader::VulkanShader(std::shared_ptr<VulkanDevice> device,
                           const std::pair<std::string, std::string> &shaderPaths) : m_Device(device) {

    auto vertShaderCode = ReadFile(shaderPaths.first);
    auto fragShaderCode = ReadFile(shaderPaths.second);

    VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    std::vector<VkPushConstantRange> pushConstantRanges;
    VkVertexInputBindingDescription bindingDescription{};
    std::map<uint32_t, DescriptorSetLayoutData> setLayoutsMap;
    for (auto code: {vertShaderCode, fragShaderCode}) {
        SpvReflectShaderModule module = {};
        SpvReflectResult result = spvReflectCreateShaderModule(code.size(),
                                                               reinterpret_cast<const uint32_t *>(code.data()),
                                                               &module);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        uint32_t count = 0;
        result = spvReflectEnumerateInputVariables(&module, &count, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectInterfaceVariable *> inputVars(count);
        result = spvReflectEnumerateInputVariables(&module, &count, inputVars.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        count = 0;
        result = spvReflectEnumerateOutputVariables(&module, &count, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectInterfaceVariable *> outputVars(count);
        result = spvReflectEnumerateOutputVariables(&module, &count, outputVars.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        count = 0;
        result = spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectDescriptorSet *> sets(count);
        result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        count = 0;
        result = spvReflectEnumeratePushConstantBlocks(&module, &count, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectBlockVariable *> pushConstantBlocks(count);
        result = spvReflectEnumeratePushConstantBlocks(&module, &count, pushConstantBlocks.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        // Get VerterInputDescription
        if (module.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) {
            bindingDescription.binding = 0;
            bindingDescription.stride = 0;  // computed below
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            attributeDescriptions.reserve(inputVars.size());
            for (auto &inputVar: inputVars) {
                const SpvReflectInterfaceVariable &reflVar = *inputVar;
                // ignore built-in variables
                if (reflVar.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
                    continue;
                }

                VkVertexInputAttributeDescription attr_desc{};
                attr_desc.location = reflVar.location;
                attr_desc.binding = bindingDescription.binding;
                attr_desc.format = static_cast<VkFormat>(reflVar.format);
                attr_desc.offset = 0;  // final offset computed below after sorting.

                attributeDescriptions.push_back(attr_desc);
            }

            // Sort attributes by location
            std::sort(std::begin(attributeDescriptions),
                      std::end(attributeDescriptions),
                      [](const VkVertexInputAttributeDescription &a,
                         const VkVertexInputAttributeDescription &b) {
                          return a.location < b.location;
                      });

            // Compute final offsets of each attribute, and total vertex stride.
            for (auto &attribute: attributeDescriptions) {
                // TODO: Check why FormatSize wasn't working correctly.
                // uint32_t format_size = FormatSize(attribute.format);
                uint32_t format_size = 16;
                if (attribute.format == VK_FORMAT_R32G32_SFLOAT)
                    format_size = 8;

                attribute.offset = bindingDescription.stride;
                bindingDescription.stride += format_size;
            }
        }

        for (auto &set: sets) {
            const SpvReflectDescriptorSet &reflSet = *set;
            DescriptorSetLayoutData &setLayout = setLayoutsMap[reflSet.set];
            for (uint32_t iBinding = 0; iBinding < reflSet.binding_count; ++iBinding) {
                const SpvReflectDescriptorBinding &reflBinding = *(reflSet.bindings[iBinding]);
                VkDescriptorSetLayoutBinding &layoutBinding = setLayout.bindings[reflBinding.binding]; // Get correct index from map
                layoutBinding.binding = reflBinding.binding;
                layoutBinding.descriptorType = static_cast<VkDescriptorType>(reflBinding.descriptor_type);
                layoutBinding.descriptorCount = 1;
                for (uint32_t iDim = 0; iDim < reflBinding.array.dims_count; ++iDim) {
                    layoutBinding.descriptorCount *= reflBinding.array.dims[iDim];
                }
                layoutBinding.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);
            }
        }

        for (auto &pushConstantBlock: pushConstantBlocks) {
            VkPushConstantRange pushConstant;
            pushConstant.offset = pushConstantBlock->offset;
            pushConstant.size = pushConstantBlock->size;
            pushConstant.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);
            pushConstantRanges.push_back(pushConstant);
        }
    }

    for (auto &[set, bindingMap]: setLayoutsMap) {
        std::vector<VkDescriptorSetLayoutBinding> bindingsLayouts;
        std::transform(bindingMap.bindings.begin(), bindingMap.bindings.end(),
                       std::back_inserter(bindingsLayouts), [](const auto &pair) { return pair.second; });

        bindingMap.createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        bindingMap.createInfo.bindingCount = bindingsLayouts.size();
        bindingMap.createInfo.pBindings = bindingsLayouts.data(); // To be filled later
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShaderModule,
            .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShaderModule,
            .pName = "main",
    };

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

    vkDestroyShaderModule(m_Device->GetDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(m_Device->GetDevice(), vertShaderModule, nullptr);

    m_DescriptorSetLayouts.resize(setLayoutsMap.size());
    for (const auto &[set, setLayout]: setLayoutsMap) {
        VK_CHECK(vkCreateDescriptorSetLayout(m_Device->GetDevice(), &setLayout.createInfo, nullptr,
                                             &m_DescriptorSetLayouts[set]),
                 "Failed to create descriptor set layout!");
    }
}

VkShaderModule VulkanShader::CreateShaderModule(const std::vector<char> &code) const {
    VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t *>(code.data()),
    };

    VkShaderModule shaderModule;
    VK_CHECK(vkCreateShaderModule(m_Device->GetDevice(), &createInfo, nullptr, &shaderModule),
             "Failed to create shader module!");
    return shaderModule;
}
