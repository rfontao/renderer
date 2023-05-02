#pragma once

#include "Camera.h"
#include "Vulkan/VulkanPipeline.h"
#include "vulkan/VulkanTexture.h"
#include "vulkan/VulkanDevice.h"
#include "vulkan/VulkanSwapchain.h"
#include "vulkan/VulkanImage.h"
#include "vulkan/VulkanBuffer.h"
#include "vulkan/Utils.h"

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};


struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex &other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<>
    struct hash<Vertex> {
        size_t operator()(Vertex const &vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                     (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

class Application {
public:
    Application() = default;
    void Run();

    Camera &GetCamera();

    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void Cleanup();

    void CreateInstance();

    void SetupDebugMessenger();

    [[nodiscard]] std::vector<const char *> GetRequiredExtensions() const;

    bool CheckValidationLayerSupport();

    [[nodiscard]] VkSurfaceKHR CreateSurface() const;

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void DrawFrame();

    void CreateVertexBuffer();

    void CreateIndexBuffer();

    void CreateUniformBuffers();

    void UpdateUniformBuffer(uint32_t currentImage);

    void CreateDescriptorPool();

    void CreateDescriptorSets();

    void CreateDepthResources();

    void LoadModel();

    void CreateColorResources();

    std::shared_ptr<VulkanDevice> m_Device;
    VkInstance m_Instance;
    std::shared_ptr<VulkanTexture> texture;

    std::shared_ptr<VulkanImage> depthImage;
    std::shared_ptr<VulkanImage> colorImage;

    std::shared_ptr<VulkanBuffer> vertexBuffer;
    std::shared_ptr<VulkanBuffer> indexBuffer;

    std::shared_ptr<VulkanSwapchain> m_Swapchain;

    std::shared_ptr<VulkanPipeline> m_GraphicsPipeline;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkDebugUtilsMessengerEXT debugMessenger;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<std::shared_ptr<VulkanBuffer>> uniformBuffers;

    uint32_t currentFrame = 0;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    Camera m_Camera;
    GLFWwindow *m_Window;

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const std::string MODEL_PATH = "models/viking_room.obj";
    const std::string TEXTURE_PATH = "textures/viking_room.png";

    const std::vector<const char *> validationLayers = {
//            "VK_LAYER_LUNARG_api_dump",
            "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

};
