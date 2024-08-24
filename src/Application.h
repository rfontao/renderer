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

struct DirectionalLightUBO {
    glm::mat4 view;
    glm::mat4 proj;
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

    void FindScenePaths(const std::filesystem::path &basePath);

    void SetScene(const std::filesystem::path &scenePath);

    void ChangeScene();

    [[nodiscard]] std::vector<std::filesystem::path> GetScenePaths() const { return m_ScenePaths; };

    void CreateInstance();

    [[nodiscard]] VkSurfaceKHR CreateSurface() const;

    void SetupDebugMessenger();

    [[nodiscard]] std::vector<const char *> GetRequiredExtensions() const;

    bool CheckValidationLayerSupport();

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void DrawFrame();

    void CreateUniformBuffers();

    void UpdateUniformBuffer(uint32_t currentImage);

    void CreateBindlessTexturesArray();

    void CreateDescriptorSets();

    void CreateDepthResources();

    void CreateColorResources();

    void HandleKeys();

    std::shared_ptr<VulkanDevice> m_Device;
    VkInstance m_Instance;

    std::shared_ptr<VulkanImage> m_DepthImage;
    std::shared_ptr<VulkanImage> m_ColorImage;

    std::shared_ptr<VulkanSwapchain> m_Swapchain;
    std::shared_ptr<VulkanPipeline> m_GraphicsPipeline;
    std::shared_ptr<VulkanPipeline> m_SkyboxPipeline;

    std::vector<VkDescriptorSet> m_DescriptorSets;
    VkDescriptorSet m_SkyboxDescriptorSet;
    std::vector<std::shared_ptr<VulkanBuffer>> m_UniformBuffers;

    VkDescriptorSet m_BindlessTexturesSet;
    std::vector<VkDescriptorImageInfo> m_TextureDescriptors;

    VkDebugUtilsMessengerEXT m_DebugMessenger;
//    VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_8_BIT;

    uint32_t m_CurrentFrame = 0;

    std::vector<std::filesystem::path> m_ScenePaths;
    std::filesystem::path m_NextScenePath;
    bool m_ShouldChangeScene = false;
    Scene m_Scene;
    Scene m_Skybox;
    UI m_UI;

    std::shared_ptr<VulkanTexture> m_CubemapTexture;

    Camera m_Camera;
    GLFWwindow *m_Window;

    const uint32_t WIDTH = 1280;
    const uint32_t HEIGHT = 800;

    const std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

    // TODO: Extract later -> probably when render graph is available
    // shadow mapping things
    constexpr static uint32_t shadowSize { 2048 };
    constexpr static float shadowDepthBias { 1.25f };

    std::shared_ptr<VulkanTexture> m_ShadowDepthTexture;

    std::shared_ptr<VulkanPipeline> m_ShadowMapPipeline;

    VkDescriptorSet m_LightDescriptorSet;

    std::shared_ptr<VulkanBuffer> m_ShadowMapUBOBuffer;

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

};
