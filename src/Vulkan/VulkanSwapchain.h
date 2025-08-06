#pragma once

#include "VulkanDevice.h"
#include "VulkanImage.h"

#include <VkBootstrap.h>

class VulkanImage;
class VulkanDevice;

constexpr uint32_t requestedFramesInFlight = 2;

class VulkanSwapchain {
public:
    VulkanSwapchain() = default;
    VulkanSwapchain(std::shared_ptr<VulkanDevice> device, GLFWwindow *window);
    void Destroy();

    void Recreate();
    uint32_t AcquireNextImage(uint32_t currentFrame);
    bool Present(uint32_t imageIndex, uint32_t currentFrame);

    [[nodiscard]] const std::vector<VkFence> &GetWaitFences() const { return waitFences; }
    [[nodiscard]] const std::vector<VkSemaphore> &
    GetImageAvailableSemaphores() const { return imageAvailableSemaphores; }
    [[nodiscard]] const std::vector<VkSemaphore> &
    GetRenderFinishedSemaphores() const { return renderFinishedSemaphores; }
    [[nodiscard]] const std::vector<VkCommandBuffer> &GetCommandBuffers() const { return commandBuffers; }
    [[nodiscard]] VkImageView GetImageView(uint32_t index) const;
    [[nodiscard]] std::shared_ptr<VulkanImage> GetImage(uint32_t index) const;
    [[nodiscard]] uint32_t GetHeight() const { return swapchain.extent.height; }
    [[nodiscard]] uint32_t GetWidth() const { return swapchain.extent.width; }
    [[nodiscard]] VkExtent2D GetExtent() const { return swapchain.extent; }
    [[nodiscard]] VkFormat GetImageFormat() const { return swapchain.image_format; }

    uint32_t numFramesInFlight {0};
    bool needsResizing = false;
private:
    void Create();

    static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    vkb::Swapchain swapchain;
    std::vector<std::shared_ptr<VulkanImage>> images;

    GLFWwindow *window {nullptr};

    std::vector<VkFence> waitFences;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    std::vector<VkCommandBuffer> commandBuffers;

    std::shared_ptr<VulkanDevice> device;
};
