#include "VulkanSwapchain.h"
#include "pch.h"

#include <utility>

#include "Utils.h"
#include "VulkanDevice.h"

VulkanSwapchain::VulkanSwapchain(std::shared_ptr<VulkanDevice> device, GLFWwindow *window) :
    window(window), device(std::move(device)) {
    Create();
}

void VulkanSwapchain::Destroy() {
    for (size_t i = 0; i < numFramesInFlight; i++) {
        vkDestroySemaphore(device->GetDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device->GetDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device->GetDevice(), waitFences[i], nullptr);
    }

    for (auto &image: images) {
        image->Destroy();
    }
    vkb::destroy_swapchain(swapchain);
}

void VulkanSwapchain::Recreate() {
    // Handle window resizing
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    // Handle minimization of window
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device->GetDevice());

    vkb::SwapchainBuilder swapchainBuilder{device->GetDevice()};
    auto swapRet = swapchainBuilder.set_old_swapchain(swapchain)
                           .set_desired_min_image_count(requestedFramesInFlight)
                           .build();
    if (!swapRet) {
        // If it failed to create a swapchain, the old swapchain handle is invalid.
        swapchain.swapchain = VK_NULL_HANDLE;
    }

    for (auto &image: images) {
        image->Destroy();
    }
    vkb::destroy_swapchain(swapchain);

    swapchain = swapRet.value();

    auto swapchainImages = swapchain.get_images().value();
    numFramesInFlight = swapchainImages.size();
    // Create swapchain images
    images.resize(numFramesInFlight);
    for (size_t i = 0; i < images.size(); ++i) {
        images[i] = std::make_shared<VulkanImage>(device, swapchainImages[i]);
    }
}

void VulkanSwapchain::Create() {
    vkb::SwapchainBuilder swapchainBuilder{device->GetDevice()};
    swapchainBuilder.set_desired_min_image_count(requestedFramesInFlight);
    auto swapRet = swapchainBuilder.build();
    if (!swapRet) {
        throw std::runtime_error("Failed to build swapchain");
    }
    swapchain = swapRet.value();

    // Create swapchain images
    auto swapchainImages = swapchain.get_images().value();
    numFramesInFlight = swapchainImages.size();
    images.resize(swapchainImages.size());
    for (size_t i = 0; i < images.size(); ++i) {
        images[i] = std::make_shared<VulkanImage>(device, swapchainImages[i]);
    }

    // Sync objects
    imageAvailableSemaphores.resize(numFramesInFlight);
    renderFinishedSemaphores.resize(numFramesInFlight);
    waitFences.resize(numFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT, // Used to no hand program on first frame draw
    };

    for (size_t i = 0; i < numFramesInFlight; ++i) {
        if (vkCreateSemaphore(device->GetDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
                    VK_SUCCESS ||
            vkCreateSemaphore(device->GetDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
                    VK_SUCCESS ||
            vkCreateFence(device->GetDevice(), &fenceInfo, nullptr, &waitFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain sync objects!");
        }
    }

    // Command buffers
    commandBuffers.resize(numFramesInFlight);

    VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = device->GetCommandPool(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = (uint32_t) commandBuffers.size(),
    };

    VK_CHECK(vkAllocateCommandBuffers(device->GetDevice(), &allocInfo, commandBuffers.data()),
             "Failed to allocate command buffers!");
}

uint32_t VulkanSwapchain::AcquireNextImage(uint32_t currentFrame) {
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device->GetDevice(), swapchain, UINT64_MAX,
                                            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

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
    VkSwapchainKHR swapChains[] = {swapchain};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = signalSemaphores,
            .swapchainCount = 1,
            .pSwapchains = swapChains,
            .pImageIndices = &imageIndex,
    };

    VkResult result = vkQueuePresentKHR(device->GetPresentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || needsResizing) {
        needsResizing = false;
        Recreate();
        return true;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    return false;
}

std::shared_ptr<VulkanImage> VulkanSwapchain::GetImage(uint32_t index) const { return images[index]; }

VkImageView VulkanSwapchain::GetImageView(uint32_t index) const { return images[index]->GetImageView(); }

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
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {(uint32_t) width, (uint32_t) height};

        actualExtent.width =
                std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height =
                std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
