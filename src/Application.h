#pragma once

#include "GPUDataUploader.h"
#include "Scene.h"
#include "UI/UI.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanImage.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanSwapchain.h"
#include "Vulkan/VulkanTexture.h"

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

    [[nodiscard]] std::vector<std::filesystem::path> GetScenePaths() const { return scenePaths; };

    void CreateInstance();
    [[nodiscard]] VkSurfaceKHR CreateSurface() const;

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void DrawFrame();

    void UpdateUniformBuffer(uint32_t currentImage);

    void CreateBindlessTexturesArray();
    void CreateDepthResources();
    void CreateColorResources();

    void HandleKeys();

    std::shared_ptr<VulkanDevice> device;
    vkb::Instance instance;

    std::shared_ptr<VulkanImage> depthImage;
    std::shared_ptr<VulkanImage> colorImage;

    std::shared_ptr<VulkanSwapchain> swapchain;
    std::shared_ptr<VulkanPipeline> graphicsPipeline;
    std::shared_ptr<VulkanPipeline> skyboxPipeline;

    VkDescriptorSet bindlessTexturesSet;
    std::vector<VkDescriptorImageInfo> textureDescriptors;

    uint32_t currentFrame = 0;

    std::vector<std::filesystem::path> scenePaths;
    std::filesystem::path nextScenePath;
    bool shouldChangeScene = false;
    std::unique_ptr<Scene> scene;
    std::unique_ptr<Scene> skybox;
    UI userInterface;

    std::shared_ptr<TextureCube> cubemapTexture;

    GLFWwindow *window;

    const uint32_t WIDTH = 1280;
    const uint32_t HEIGHT = 800;

    // TODO: Extract later -> probably when render graph is available
    // Shadow Mapping
    constexpr static uint32_t shadowSize{4096};
    constexpr static float shadowDepthBias{2.00f};
    constexpr static float shadowDepthSlope{1.0f};

    static constexpr bool frustumCulling{false};

    std::shared_ptr<Texture2D> shadowDepthTexture;
    std::shared_ptr<VulkanPipeline> shadowMapPipeline;

    std::shared_ptr<VulkanPipeline> debugDrawPipeline;
    std::shared_ptr<VulkanPipeline> frustumCullingPipeline;

    GPUDataUploader GPUDataUploader;
    std::unique_ptr<DebugDraw> debugDraw;

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
};
