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
    glm::vec4 viewPos;
};

class UI;
class Application {
public:
    Application() = default;

    void Run();

    Camera &GetCamera() { return m_Camera; }

    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void Cleanup();

    void FindScenePaths(const std::filesystem::path& basePath);
    void SetScene(const std::filesystem::path& scenePath);
    void ChangeScene();
    const std::vector<std::filesystem::path>& GetScenePaths() const { return m_ScenePaths; };

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

    std::vector<std::filesystem::path> m_ScenePaths;
    std::filesystem::path m_NextScenePath;
    bool m_ShouldChangeScene = false;
    Scene m_Scene;
    UI m_UI;

    Camera m_Camera;
    GLFWwindow *m_Window;

    const uint32_t WIDTH = 1280;
    const uint32_t HEIGHT = 800;

    const std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    void HandleKeys();
};
