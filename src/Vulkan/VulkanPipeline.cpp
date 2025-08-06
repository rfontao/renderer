#include "pch.h"

#include <utility>

#include "VulkanPipeline.h"
#include "spirv_reflect.h"
#include "Utils.h"

// https://github.com/KhronosGroup/SPIRV-Reflect/blob/master/examples/main_io_variables.cpp
static uint32_t FormatSize(VkFormat format) {
    uint32_t result = 0;
    switch (format) {
        case VK_FORMAT_UNDEFINED:
            result = 0;
            break;
        case VK_FORMAT_R4G4_UNORM_PACK8:
            result = 1;
            break;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_R8_UNORM:
            result = 1;
            break;
        case VK_FORMAT_R8_SNORM:
            result = 1;
            break;
        case VK_FORMAT_R8_USCALED:
            result = 1;
            break;
        case VK_FORMAT_R8_SSCALED:
            result = 1;
            break;
        case VK_FORMAT_R8_UINT:
            result = 1;
            break;
        case VK_FORMAT_R8_SINT:
            result = 1;
            break;
        case VK_FORMAT_R8_SRGB:
            result = 1;
            break;
        case VK_FORMAT_R8G8_UNORM:
            result = 2;
            break;
        case VK_FORMAT_R8G8_SNORM:
            result = 2;
            break;
        case VK_FORMAT_R8G8_USCALED:
            result = 2;
            break;
        case VK_FORMAT_R8G8_SSCALED:
            result = 2;
            break;
        case VK_FORMAT_R8G8_UINT:
            result = 2;
            break;
        case VK_FORMAT_R8G8_SINT:
            result = 2;
            break;
        case VK_FORMAT_R8G8_SRGB:
            result = 2;
            break;
        case VK_FORMAT_R8G8B8_UNORM:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8_SNORM:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8_USCALED:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8_SSCALED:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8_UINT:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8_SINT:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8_SRGB:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_UNORM:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_SNORM:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_USCALED:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_SSCALED:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_UINT:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_SINT:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_SRGB:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8A8_UNORM:
            result = 4;
            break;
        case VK_FORMAT_R8G8B8A8_SNORM:
            result = 4;
            break;
        case VK_FORMAT_R8G8B8A8_USCALED:
            result = 4;
            break;
        case VK_FORMAT_R8G8B8A8_SSCALED:
            result = 4;
            break;
        case VK_FORMAT_R8G8B8A8_UINT:
            result = 4;
            break;
        case VK_FORMAT_R8G8B8A8_SINT:
            result = 4;
            break;
        case VK_FORMAT_R8G8B8A8_SRGB:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_UNORM:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_SNORM:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_USCALED:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_SSCALED:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_UINT:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_SINT:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_SRGB:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_R16_UNORM:
            result = 2;
            break;
        case VK_FORMAT_R16_SNORM:
            result = 2;
            break;
        case VK_FORMAT_R16_USCALED:
            result = 2;
            break;
        case VK_FORMAT_R16_SSCALED:
            result = 2;
            break;
        case VK_FORMAT_R16_UINT:
            result = 2;
            break;
        case VK_FORMAT_R16_SINT:
            result = 2;
            break;
        case VK_FORMAT_R16_SFLOAT:
            result = 2;
            break;
        case VK_FORMAT_R16G16_UNORM:
            result = 4;
            break;
        case VK_FORMAT_R16G16_SNORM:
            result = 4;
            break;
        case VK_FORMAT_R16G16_USCALED:
            result = 4;
            break;
        case VK_FORMAT_R16G16_SSCALED:
            result = 4;
            break;
        case VK_FORMAT_R16G16_UINT:
            result = 4;
            break;
        case VK_FORMAT_R16G16_SINT:
            result = 4;
            break;
        case VK_FORMAT_R16G16_SFLOAT:
            result = 4;
            break;
        case VK_FORMAT_R16G16B16_UNORM:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16_SNORM:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16_USCALED:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16_SSCALED:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16_UINT:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16_SINT:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16_SFLOAT:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16A16_UNORM:
            result = 8;
            break;
        case VK_FORMAT_R16G16B16A16_SNORM:
            result = 8;
            break;
        case VK_FORMAT_R16G16B16A16_USCALED:
            result = 8;
            break;
        case VK_FORMAT_R16G16B16A16_SSCALED:
            result = 8;
            break;
        case VK_FORMAT_R16G16B16A16_UINT:
            result = 8;
            break;
        case VK_FORMAT_R16G16B16A16_SINT:
            result = 8;
            break;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            result = 8;
            break;
        case VK_FORMAT_R32_UINT:
            result = 4;
            break;
        case VK_FORMAT_R32_SINT:
            result = 4;
            break;
        case VK_FORMAT_R32_SFLOAT:
            result = 4;
            break;
        case VK_FORMAT_R32G32_UINT:
            result = 8;
            break;
        case VK_FORMAT_R32G32_SINT:
            result = 8;
            break;
        case VK_FORMAT_R32G32_SFLOAT:
            result = 8;
            break;
        case VK_FORMAT_R32G32B32_UINT:
            result = 12;
            break;
        case VK_FORMAT_R32G32B32_SINT:
            result = 12;
            break;
        case VK_FORMAT_R32G32B32_SFLOAT:
            result = 12;
            break;
        case VK_FORMAT_R32G32B32A32_UINT:
            result = 16;
            break;
        case VK_FORMAT_R32G32B32A32_SINT:
            result = 16;
            break;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            result = 16;
            break;
        case VK_FORMAT_R64_UINT:
            result = 8;
            break;
        case VK_FORMAT_R64_SINT:
            result = 8;
            break;
        case VK_FORMAT_R64_SFLOAT:
            result = 8;
            break;
        case VK_FORMAT_R64G64_UINT:
            result = 16;
            break;
        case VK_FORMAT_R64G64_SINT:
            result = 16;
            break;
        case VK_FORMAT_R64G64_SFLOAT:
            result = 16;
            break;
        case VK_FORMAT_R64G64B64_UINT:
            result = 24;
            break;
        case VK_FORMAT_R64G64B64_SINT:
            result = 24;
            break;
        case VK_FORMAT_R64G64B64_SFLOAT:
            result = 24;
            break;
        case VK_FORMAT_R64G64B64A64_UINT:
            result = 32;
            break;
        case VK_FORMAT_R64G64B64A64_SINT:
            result = 32;
            break;
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            result = 32;
            break;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            result = 4;
            break;

        default:
            break;
    }
    return result;
}

VulkanPipeline::VulkanPipeline(std::shared_ptr<VulkanDevice> device, PipelineSpecification &pipelineSpecification) :
    spec(pipelineSpecification), device(std::move(device)) {

    bool isCompute = !pipelineSpecification.compShaderPath.empty();
    std::vector<std::vector<char>> shaderSources;
    if (!isCompute) {
        auto vertShaderCode = ReadFile(pipelineSpecification.vertShaderPath);
        auto fragShaderCode = ReadFile(pipelineSpecification.fragShaderPath);
        shaderSources.push_back(vertShaderCode);
        shaderSources.push_back(fragShaderCode);
    } else {
        shaderSources.push_back(ReadFile(pipelineSpecification.compShaderPath));
    }

    std::unordered_map<uint32_t, DescriptorSetLayoutData> setLayouts;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    std::vector<VkPushConstantRange> pushConstantRanges;
    VkVertexInputBindingDescription bindingDescription{};
    for (auto code: shaderSources) {
        SpvReflectShaderModule module = {};
        SpvReflectResult result = spvReflectCreateShaderModule(code.size(), code.data(), &module);
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

        if (module.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) {
            bindingDescription.binding = 0;
            bindingDescription.stride = 0; // computed below
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
                attr_desc.offset = 0; // final offset computed below after sorting.

                attributeDescriptions.push_back(attr_desc);
            }

            // Sort attributes by location
            std::sort(std::begin(attributeDescriptions), std::end(attributeDescriptions),
                      [](const auto &a, const auto &b) { return a.location < b.location; });

            // Compute final offsets of each attribute, and total vertex stride.
            for (auto &attribute: attributeDescriptions) {
                // TODO: Change
                // uint32_t format_size = FormatSize(attribute.format);
                uint32_t format_size = 16;
                if (attribute.format == VK_FORMAT_R32G32_SFLOAT)
                    format_size = 8;

                attribute.offset = bindingDescription.stride;
                bindingDescription.stride += format_size;
            }
        }

        // TODO: Handle sets and binding defined in different shader files
        for (auto &set: sets) {
            const SpvReflectDescriptorSet &reflSet = *set;

            DescriptorSetLayoutData layout = setLayouts[reflSet.set];
            if (layout.bindings.size() < reflSet.binding_count) {
                layout.bindings.resize(reflSet.binding_count);
            }

            for (uint32_t iBinding = 0; iBinding < reflSet.binding_count; ++iBinding) {
                const SpvReflectDescriptorBinding &reflBinding = *(reflSet.bindings[iBinding]);
                VkDescriptorSetLayoutBinding &layoutBinding = layout.bindings[iBinding];
                layoutBinding.binding = reflBinding.binding;
                layoutBinding.descriptorType = static_cast<VkDescriptorType>(reflBinding.descriptor_type);

                layoutBinding.descriptorCount = 1;
                for (uint32_t iDim = 0; iDim < reflBinding.array.dims_count; ++iDim) {
                    layoutBinding.descriptorCount *= reflBinding.array.dims[iDim];
                }
                // NOTE: Bindless case
                if (layoutBinding.descriptorCount == 0) {
                    layoutBinding.descriptorCount = 1000;
                }

                layoutBinding.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);
            }
            layout.setNumber = reflSet.set;
            layout.createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout.createInfo.bindingCount = reflSet.binding_count;
            layout.createInfo.pBindings = layout.bindings.data();

            setLayouts[reflSet.set] = layout;
        }

        for (auto &pushConstantBlock: pushConstantBlocks) {
            VkPushConstantRange pushConstant;
            pushConstant.offset = pushConstantBlock->offset;
            pushConstant.size = pushConstantBlock->size;
            pushConstant.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);
            pushConstantRanges.push_back(pushConstant);
        }
    }

    descriptorSetLayouts.resize(setLayouts.size());
    for (int i = 0; i < setLayouts.size(); ++i) {
        VkDescriptorSetLayoutCreateInfo layoutInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = (uint32_t) setLayouts[i].bindings.size(),
                .pBindings = setLayouts[i].bindings.data(),
        };

        VkDescriptorBindingFlags flags =
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

        // Must be outside, otherwise crashes :)
        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{};
        bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        bindingFlags.pNext = nullptr;
        bindingFlags.pBindingFlags = &flags;
        bindingFlags.bindingCount = 1;

        if (i == 0 && (pipelineSpecification.fragShaderPath == "shaders/pbr_bindless.frag.spv" ||
                       pipelineSpecification.fragShaderPath == "shaders/skybox.frag.spv")) {
            layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
            layoutInfo.pNext = &bindingFlags;
        }

        VK_CHECK(vkCreateDescriptorSetLayout(this->device->GetDevice(), &layoutInfo, nullptr, &descriptorSetLayouts[i]),
                 "Failed to create descriptor set layout!");
    }

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkShaderModule> shaderModules;
    if (!isCompute) {
        VkShaderModule vertShaderModule = CreateShaderModule(shaderSources[0]);
        VkShaderModule fragShaderModule = CreateShaderModule(shaderSources[1]);
        shaderModules.push_back(vertShaderModule);
        shaderModules.push_back(fragShaderModule);

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

        shaderStages.push_back(vertShaderStageInfo);
        shaderStages.push_back(fragShaderStageInfo);
    } else {
        VkShaderModule compShaderModule = CreateShaderModule(shaderSources[0]);
        shaderModules.push_back(compShaderModule);

        VkPipelineShaderStageCreateInfo compShaderStageInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = compShaderModule,
                .pName = "main",
        };

        shaderStages.push_back(compShaderStageInfo);
    }

    std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_DEPTH_BIAS // NOTE: For shadow mapping
    };

    VkPipelineDynamicStateCreateInfo dynamicState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = (uint32_t) dynamicStates.size(),
            .pDynamicStates = dynamicStates.data(),
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &bindingDescription,
            .vertexAttributeDescriptionCount = (uint32_t) attributeDescriptions.size(),
            .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = spec.wireframe ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo viewportState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1,
    };

    VkCullModeFlags cullMode;
    switch (pipelineSpecification.cullingMode) {
        case CullingMode::NONE:
            cullMode = VK_CULL_MODE_NONE;
            break;
        case CullingMode::FRONT:
            cullMode = VK_CULL_MODE_FRONT_BIT;
            break;
        case CullingMode::BACK:
            cullMode = VK_CULL_MODE_BACK_BIT;
            break;
        case CullingMode::FRONT_AND_BACK:
            cullMode = VK_CULL_MODE_FRONT_AND_BACK;
            break;
    }

    VkPipelineRasterizationStateCreateInfo rasterizer{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = cullMode,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = pipelineSpecification.depthBiasEnable,
            .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment{
            .blendEnable = pipelineSpecification.blendEnable,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = pipelineSpecification.enableDepthTesting,
            .depthWriteEnable = pipelineSpecification.enableDepthTesting,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
            .pSetLayouts = descriptorSetLayouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
            .pPushConstantRanges = pushConstantRanges.data(),
    };

    VK_CHECK(vkCreatePipelineLayout(this->device->GetDevice(), &pipelineLayoutInfo, nullptr, &layout),
             "Failed to create pipeline layout!");

    // Dynamic rendering
    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    // TODO: Reevaluate
    VkFormat colorAttachmentFormat{VK_FORMAT_B8G8R8A8_SRGB};
    if (pipelineSpecification.depthBiasEnable) { // Means shadowmapping
        pipelineRenderingInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount = 0,
                .depthAttachmentFormat = VK_FORMAT_D16_UNORM,
        };
    } else {
        pipelineRenderingInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &colorAttachmentFormat,
                .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
        };
    }


    if (isCompute) {
        VkComputePipelineCreateInfo pipelineInfo{
                .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                .stage = shaderStages[0],
                .layout = layout,
        };
        VK_CHECK(
                vkCreateComputePipelines(this->device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
                "Failed to create graphics pipeline!");
    } else {
        VkGraphicsPipelineCreateInfo pipelineInfo{
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &pipelineRenderingInfo,
                .stageCount = static_cast<uint32_t>(shaderStages.size()),
                .pStages = shaderStages.data(),
                .pVertexInputState = &vertexInputInfo,
                .pInputAssemblyState = &inputAssembly,
                .pViewportState = &viewportState,
                .pRasterizationState = &rasterizer,
                .pMultisampleState = &multisampling,
                .pDepthStencilState = &depthStencil,
                .pColorBlendState = &colorBlending,
                .pDynamicState = &dynamicState,
                .layout = layout,
                // .renderPass = renderPass, -> no longer needed because of dynamic rendering
                .renderPass = VK_NULL_HANDLE,
                .subpass = 0,
                .basePipelineHandle = VK_NULL_HANDLE,
        };

        VK_CHECK(vkCreateGraphicsPipelines(this->device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                           &pipeline),
                 "Failed to create graphics pipeline!");
    }


    for (const auto &shaderModule: shaderModules) {
        vkDestroyShaderModule(this->device->GetDevice(), shaderModule, nullptr);
    }
}

VkShaderModule VulkanPipeline::CreateShaderModule(const std::vector<char> &code) const {
    VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t *>(code.data()),
    };

    VkShaderModule shaderModule;
    VK_CHECK(vkCreateShaderModule(device->GetDevice(), &createInfo, nullptr, &shaderModule),
             "Failed to create shader module!");
    return shaderModule;
}

void VulkanPipeline::Destroy() {
    for (auto& descriptorSetLayout: descriptorSetLayouts) {
        vkDestroyDescriptorSetLayout(device->GetDevice(), descriptorSetLayout, nullptr);
    }
    vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
    vkDestroyPipelineLayout(device->GetDevice(), layout, nullptr);
}
