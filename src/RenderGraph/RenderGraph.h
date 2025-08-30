#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Vulkan/VulkanImage.h"


struct TextureSpecification;
namespace RG {

    // NOTES TO MYSELF: não deve ser preciso gerar o rendergraph todos os frames porque eles sera bastante estatica
    //  Posso "simular" placed resources com o VMA portanto posso alocar e dealocar todos os recursos por frame

    struct ResourceHandle {
        enum class Type { Texture, Buffer } type;
        uint32_t id; // Unique identifier for the resource
    };

    class ResourceRegistry {
        std::vector<ResourceHandle> resources;
    };

    enum class ResourceUsageType {
        READ_ONLY,
        WRITE_ONLY,
        READ_WRITE,
    };

    enum class AttachmentClearPolicy {
        READ_ONLY,
        WRITE_ONLY,
        READ_WRITE,
    };

    class RenderGraphBuilder {

        ResourceHandle CreateTexture(const TextureSpecification& specification);

        ResourceHandle AddColorAttachment();
        ResourceHandle AddDepthStencilAttachment();

        ResourceHandle AddTexture();
    };

    class RenderGraphExecutionContext {
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
        uint32_t frameIndex{0};
    };

    class RenderGraphPassBase {
        explicit RenderGraphPassBase(std::string name) : name(std::move(name)) {};
        virtual void Setup(RenderGraphBuilder &builder) = 0;
        virtual void Execute(RenderGraphExecutionContext &context) = 0;
        virtual ~RenderGraphPassBase() = default;

        std::string name;
    };

    class RenderGraph {
    public:
        void AddPass(std::unique_ptr<RenderGraphPassBase> pass);
    };

} // namespace RG
