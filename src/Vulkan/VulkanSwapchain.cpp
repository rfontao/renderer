#include "pch.h"
#include "VulkanSwapchain.h"

#include <utility>

#include "VulkanDevice.h"
#include "Utils.h"

VulkanSwapchain::VulkanSwapchain(std::shared_ptr<VulkanDevice> device, GLFWwindow *window) : m_Window(window),
    m_Device(std::move(device)) {
    Create();
}

void VulkanSwapchain::Destroy() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(m_Device->GetDevice(), m_RenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_Device->GetDevice(), m_ImageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_Device->GetDevice(), m_WaitFences[i], nullptr);
    }

    for (auto &image: m_Images) {
        image->Destroy();
    }
    vkb::destroy_swapchain(m_Swapchain);
}

void VulkanSwapchain::Recreate() {
    // Handle window resizing
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);

    // Handle minimization of window
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_Device->GetDevice());

    vkb::SwapchainBuilder swapchainBuilder{m_Device->GetDevice()};
    auto swapRet = swapchainBuilder
            .set_old_swapchain(m_Swapchain)
            .build();
    if (!swapRet){
        // If it failed to create a swapchain, the old swapchain handle is invalid.
        m_Swapchain.swapchain = VK_NULL_HANDLE;
    }

    for (auto &image: m_Images) {
        image->Destroy();
    }
    vkb::destroy_swapchain(m_Swapchain);

    m_Swapchain = swapRet.value();

    auto swapchainImages = m_Swapchain.get_images().value();
    // Create swapchain images
    m_Images.resize(swapchainImages.size());
    for (size_t i = 0; i < m_Images.size(); ++i) {
        m_Images[i] = std::make_shared<VulkanImage>(m_Device, swapchainImages[i]);
    }
}

void VulkanSwapchain::Create() {
    const vkb::SwapchainBuilder swapchainBuilder{m_Device->GetDevice()};
    auto swapRet = swapchainBuilder.build();
    if (!swapRet) {
        throw std::runtime_error("Failed to build swapchain");
    }
    m_Swapchain = swapRet.value();

    // Create swapchain images
    auto swapchainImages = m_Swapchain.get_images().value();
    m_Images.resize(swapchainImages.size());
    for (size_t i = 0; i < m_Images.size(); ++i) {
        m_Images[i] = std::make_shared<VulkanImage>(m_Device, swapchainImages[i]);
    }

    // Sync objects
    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_WaitFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT, // Used to no hand program on first frame draw
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(m_Device->GetDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) !=
            VK_SUCCESS ||
            vkCreateSemaphore(m_Device->GetDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) !=
            VK_SUCCESS ||
            vkCreateFence(m_Device->GetDevice(), &fenceInfo, nullptr, &m_WaitFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain sync objects!");
        }
    }

    // Command buffers
    m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_Device->GetCommandPool(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (uint32_t) m_CommandBuffers.size(),
    };

    VK_CHECK(vkAllocateCommandBuffers(m_Device->GetDevice(), &allocInfo, m_CommandBuffers.data()),
             "Failed to allocate command buffers!");
}

uint32_t VulkanSwapchain::AcquireNextImage(uint32_t currentFrame) {
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_Device->GetDevice(), m_Swapchain, UINT64_MAX,
                                            m_ImageAvailableSemaphores[currentFrame],
                                            VK_NULL_HANDLE,
                                            &imageIndex);

    // Handle window resizing and minimization
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        Recreate();
        return std::numeric_limits<uint32_t>::max();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    return imageIndex;
}

bool VulkanSwapchain::Present(uint32_t imageIndex, uint32_t currentFrame) {
    VkSwapchainKHR swapChains[] = {m_Swapchain};
    VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[currentFrame]};
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex,
    };

    VkResult result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_NeedsResizing) {
        m_NeedsResizing = false;
        Recreate();
        return true;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    return false;
}

std::shared_ptr<VulkanImage> VulkanSwapchain::GetImage(uint32_t index) const {
    return m_Images[index];
}

VkImageView VulkanSwapchain::GetImageView(uint32_t index) const {
    return m_Images[index]->GetImageView();
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat: availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode: availablePresentModes) {
        // Triple buffering
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // Double buffering
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(m_Window, &width, &height);

        VkExtent2D actualExtent = {
            (uint32_t) width,
            (uint32_t) height
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
