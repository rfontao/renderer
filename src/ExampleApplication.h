#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vector>
#include <optional>
#include <array>
#include <string>

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

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] bool IsComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class ExampleApplication {
public:
    void Run();

private:
    void InitWindow();

    void InitVulkan();

    void MainLoop();

    void Cleanup();

    void CreateInstance();

    void CreateLogicalDevice();

    void CreateSwapChain();

    void SetupDebugMessenger();

    [[nodiscard]] std::vector<const char *> GetRequiredExtensions() const;

    bool CheckValidationLayerSupport();

    static void PrintAvailableVulkanExtensions();

    void PickPhysicalDevice();

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

    bool IsDeviceSuitable(VkPhysicalDevice device);

    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

    void CreateSurface();

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    void CreateImageViews();

    void CreateGraphicsPipeline();

    VkShaderModule CreateShaderModule(const std::vector<char> &code);

    void CreateRenderPass();

    void CreateFramebuffers();

    void CreateCommandPool();

    void CreateCommandBuffers();

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void DrawFrame();

    void CreateSyncObjects();

    void RecreateSwapChain();

    void CleanupSwapChain();

    void CreateVertexBuffer();

    void CreateIndexBuffer();

    void CreateBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer &buffer,
                      VkDeviceMemory &bufferMemory);

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void CreateDescriptorSetLayout();

    void CreateUniformBuffers();

    void UpdateUniformBuffer(uint32_t currentImage);

    void CreateDescriptorPool();

    void CreateDescriptorSets();

    void CreateTextureImage();

    void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);

    VkCommandBuffer BeginSingleTimeCommands();

    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                               uint32_t mipLevels);

    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void CreateTextureImageView();

    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    void CreateTextureSampler();

    void CreateDepthResources();

    VkFormat
    FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    VkFormat FindDepthFormat();

    void LoadModel();

    void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    VkDebugUtilsMessengerEXT debugMessenger;

    uint32_t mipLevels;
    VkImage textureImage;
    VkImageView textureImageView;
    VkSampler textureSampler;
    VkDeviceMemory textureImageMemory;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void *> uniformBuffersMapped;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0;
    bool framebufferResized = false;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    GLFWwindow *window;

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const std::string MODEL_PATH = "models/viking_room.obj";
    const std::string TEXTURE_PATH = "textures/viking_room.png";

    const std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

};
