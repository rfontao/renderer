#pragma once

#include "Scene.h"
#include "StagingManager.h"
#include "UI/UI.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanImage.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanSwapchain.h"
#include "vulkan/VulkanTexture.h"

#include <VkBootstrap.h>

#include "DebugDraw.h"

class UI;

class Application {
public:
    Application() = default;

    void Run();

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

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void DrawFrame();

    void UpdateUniformBuffer(uint32_t currentImage);

    void CreateBindlessTexturesArray();
    void CreateDepthResources();
    void CreateColorResources();

    void HandleKeys();

    std::shared_ptr<VulkanDevice> m_Device;
    vkb::Instance m_Instance;

    std::shared_ptr<VulkanImage> m_DepthImage;
    std::shared_ptr<VulkanImage> m_ColorImage;

    std::shared_ptr<VulkanSwapchain> m_Swapchain;
    std::shared_ptr<VulkanPipeline> m_GraphicsPipeline;
    std::shared_ptr<VulkanPipeline> m_SkyboxPipeline;

    VkDescriptorSet m_BindlessTexturesSet;
    std::vector<VkDescriptorImageInfo> m_TextureDescriptors;

    uint32_t m_CurrentFrame = 0;

    std::vector<std::filesystem::path> m_ScenePaths;
    std::filesystem::path m_NextScenePath;
    bool m_ShouldChangeScene = false;
    Scene m_Scene;
    Scene m_Skybox;
    UI m_UI;

    std::shared_ptr<TextureCube> m_CubemapTexture;

    GLFWwindow *m_Window;

    const uint32_t WIDTH = 1280;
    const uint32_t HEIGHT = 800;

    // TODO: Extract later -> probably when render graph is available
    // Shadow Mapping
    constexpr static uint32_t shadowSize{4096};
    constexpr static float shadowDepthBias{2.00f};
    constexpr static float shadowDepthSlope{1.0f};

    size_t cameraIndexControlling{0};
    static constexpr size_t cameraIndexDrawing{0};

    static constexpr bool frustumCulling{false};

    std::shared_ptr<Texture2D> m_ShadowDepthTexture;
    std::shared_ptr<VulkanPipeline> m_ShadowMapPipeline;

    std::shared_ptr<VulkanPipeline> debugDrawPipeline;
    std::shared_ptr<VulkanPipeline> m_FrustumCullingPipeline;

    StagingManager stagingManager;
    std::unique_ptr<DebugDraw> debugDraw;

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
};
