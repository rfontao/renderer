#pragma once

#include "device.h"
#include "image.h"

class Image;
class Device;

class Swapchain {
public:
    Swapchain() = default;
    Swapchain(std::shared_ptr<Device> device, GLFWwindow *window);

    ~Swapchain();

    void Recreate();
    uint32_t AcquireNextImage(uint32_t currentFrame);
    void Present(uint32_t imageIndex, uint32_t currentFrame);

    [[nodiscard]] const std::vector<VkFence> &GetWaitFences() const { return m_WaitFences; }
    [[nodiscard]] const std::vector<VkSemaphore> &
    GetImageAvailableSemaphores() const { return m_ImageAvailableSemaphores; }
    [[nodiscard]] const std::vector<VkSemaphore> &
    GetRenderFinishedSemaphores() const { return m_RenderFinishedSemaphores; }
    [[nodiscard]] const std::vector<VkCommandBuffer> &GetCommandBuffers() const { return m_CommandBuffers; }
    [[nodiscard]] VkImageView GetImageView(uint32_t index) const;
    [[nodiscard]] std::shared_ptr<Image> GetImage(uint32_t index) const;
    [[nodiscard]] uint32_t GetHeight() const { return m_Extent.height; }
    [[nodiscard]] uint32_t GetWidth() const { return m_Extent.width; }
    [[nodiscard]] VkExtent2D GetExtent() const { return m_Extent; }
    [[nodiscard]] const VkFormat& GetImageFormat() const { return m_ImageFormat; }

    int MAX_FRAMES_IN_FLIGHT = 2;
    bool m_NeedsResizing = false;
private:
    void Create();
    void Destroy();

    static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    VkSwapchainKHR m_Swapchain;
    std::vector<std::shared_ptr<Image>> m_Images;

    VkFormat m_ImageFormat;
    VkExtent2D m_Extent;

    std::shared_ptr<Device> m_Device;
    GLFWwindow *m_Window;

    std::vector<VkFence> m_WaitFences;
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;

    std::vector<VkCommandBuffer> m_CommandBuffers;
    VkCommandPool m_CommandPool;
};
