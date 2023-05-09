#pragma once

#include "Camera.h"
#include "Vulkan/VulkanPipeline.h"
#include "vulkan/VulkanTexture.h"
#include "vulkan/VulkanDevice.h"
#include "vulkan/VulkanSwapchain.h"
#include "vulkan/VulkanImage.h"
#include "vulkan/VulkanBuffer.h"
#include "vulkan/Utils.h"
#include "Scene.h"
#include "UI/UI.h"

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};

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
    [[nodiscard]] VkSurfaceKHR CreateSurface() const;

    void SetupDebugMessenger();
    [[nodiscard]] std::vector<const char *> GetRequiredExtensions() const;

    bool CheckValidationLayerSupport();

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void DrawFrame();

    void CreateUniformBuffers();
    void UpdateUniformBuffer(uint32_t currentImage);

    void CreateDescriptorSets();
    void CreateDepthResources();
    void CreateColorResources();

    std::shared_ptr<VulkanDevice> m_Device;
    VkInstance m_Instance;

    std::shared_ptr<VulkanImage> depthImage;
    std::shared_ptr<VulkanImage> colorImage;

    std::shared_ptr<VulkanSwapchain> m_Swapchain;
    std::shared_ptr<VulkanPipeline> m_GraphicsPipeline;

    std::vector<VkDescriptorSet> descriptorSets;

    VkDebugUtilsMessengerEXT debugMessenger;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<std::shared_ptr<VulkanBuffer>> uniformBuffers;

    uint32_t currentFrame = 0;

    Scene m_Scene;
    UI m_UI;

    Camera m_Camera;
    GLFWwindow *m_Window;

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const std::string MODEL_PATH = "models/viking_room.obj";
    const std::string TEXTURE_PATH = "textures/viking_room.png";

    const std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

};
