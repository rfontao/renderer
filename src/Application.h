#pragma once

#include "camera.h"
#include "vulkan/texture.h"
#include "vulkan/device.h"
#include "vulkan/swapchain.h"
#include "vulkan/image.h"
#include "vulkan/buffer.h"
#include "vulkan/utils.h"

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};


struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].binding = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

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

public:
    void InitWindow();

    void InitVulkan();

    void MainLoop();

    void Cleanup();

    void CreateInstance();

    void SetupDebugMessenger();

    [[nodiscard]] std::vector<const char *> GetRequiredExtensions() const;

    bool CheckValidationLayerSupport();

    [[nodiscard]] VkSurfaceKHR CreateSurface() const;

    void CreateGraphicsPipeline();

    [[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void DrawFrame();

    void CreateVertexBuffer();

    void CreateIndexBuffer();

    void CreateDescriptorSetLayout();

    void CreateUniformBuffers();

    void UpdateUniformBuffer(uint32_t currentImage);

    void CreateDescriptorPool();

    void CreateDescriptorSets();

    void CreateDepthResources();

    [[nodiscard]] VkFormat
    FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

    [[nodiscard]] VkFormat FindDepthFormat() const;

    void LoadModel();

    void CreateColorResources();

    VkInstance m_Instance;

    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkDebugUtilsMessengerEXT debugMessenger;

    std::shared_ptr<Texture> texture;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    std::shared_ptr<Image> depthImage;
    std::shared_ptr<Image> colorImage;

    std::shared_ptr<Buffer> vertexBuffer;
    std::shared_ptr<Buffer> indexBuffer;

    std::vector<std::shared_ptr<Buffer>> uniformBuffers;

    uint32_t currentFrame = 0;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::shared_ptr<Device> m_Device;

    std::shared_ptr<Swapchain> m_Swapchain;

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
