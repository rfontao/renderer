#pragma once
#include "Scene.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/VulkanPipeline.h"

class StagingManager;
class DebugDraw {
public:
    DebugDraw() = delete;
    explicit DebugDraw(std::shared_ptr<VulkanDevice> device) : device(std::move(device)) {
        vertexBuffer = std::make_unique<Buffer>(this->device, BufferSpecification{.size = 2048, .type = BufferType::VERTEX});
    }

    void DrawLine(const glm::vec3 &start, const glm::vec3 &end, const glm::vec3 &color);
    void DrawAxis(const glm::vec3 &origin, double length, const glm::vec3 &xColor = {1.0, 0.0, 0.0},
                  const glm::vec3 &yColor = {0.0, 1.0, 0.0}, const glm::vec3 &zColor = {0.0, 0.0, 1.0});
    void DrawPoint(const glm::vec3 &coords, const glm::vec3 &color);
    void DrawSphere(const glm::vec3 &center, double radius, const glm::vec3 &color);
    // void DrawFrustum(const Frustum &frustum, const glm::vec3 &color);

    void BeginFrame();
    void Draw(VkCommandBuffer commandBuffer, StagingManager &stagingManager, const VulkanPipeline &debugPipeline,
              const Scene &scene, const VkRenderingInfo &renderingInfo);
    void EndFrame();

private:
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
    };
    std::vector<Vertex> vertices;
    std::unique_ptr<Buffer> vertexBuffer;

    std::shared_ptr<VulkanDevice> device;
};
