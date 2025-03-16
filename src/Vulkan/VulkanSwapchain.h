#pragma once

#include "VulkanDevice.h"
#include "VulkanImage.h"

#include <VkBootstrap.h>

class VulkanImage;
class VulkanDevice;

class VulkanSwapchain {
public:
    VulkanSwapchain() = default;
    VulkanSwapchain(std::shared_ptr<VulkanDevice> device, GLFWwindow *window);
    void Destroy();

    void Recreate();
    uint32_t AcquireNextImage(uint32_t currentFrame);
    bool Present(uint32_t imageIndex, uint32_t currentFrame);

    [[nodiscard]] const std::vector<VkFence> &GetWaitFences() const { return m_WaitFences; }
    [[nodiscard]] const std::vector<VkSemaphore> &
    GetImageAvailableSemaphores() const { return m_ImageAvailableSemaphores; }
    [[nodiscard]] const std::vector<VkSemaphore> &
    GetRenderFinishedSemaphores() const { return m_RenderFinishedSemaphores; }
    [[nodiscard]] const std::vector<VkCommandBuffer> &GetCommandBuffers() const { return m_CommandBuffers; }
    [[nodiscard]] VkImageView GetImageView(uint32_t index) const;
    [[nodiscard]] std::shared_ptr<VulkanImage> GetImage(uint32_t index) const;
    [[nodiscard]] uint32_t GetHeight() const { return m_Swapchain.extent.height; }
    [[nodiscard]] uint32_t GetWidth() const { return m_Swapchain.extent.width; }
    [[nodiscard]] VkExtent2D GetExtent() const { return m_Swapchain.extent; }
    [[nodiscard]] VkFormat GetImageFormat() const { return m_Swapchain.image_format; }

    int MAX_FRAMES_IN_FLIGHT = 2;
    bool m_NeedsResizing = false;
private:
    void Create();

    static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    vkb::Swapchain m_Swapchain;
    std::vector<std::shared_ptr<VulkanImage>> m_Images;

    GLFWwindow *m_Window;

    std::vector<VkFence> m_WaitFences;
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;

    std::vector<VkCommandBuffer> m_CommandBuffers;

    std::shared_ptr<VulkanDevice> m_Device;
};
