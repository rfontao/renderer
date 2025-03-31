#pragma once
#include "../pch.h"

#include "Utils.h"
#include "VkBootstrap.h"
#include "VulkanSwapchain.h"

class VulkanDevice {
public:
    VulkanDevice(vkb::Instance instance, VkSurfaceKHR surface);
    void Destroy();

    [[nodiscard]] vkb::Device GetDevice() const { return m_Device; }
    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    [[nodiscard]] VkSurfaceKHR GetSurface() const { return m_Surface; }
    [[nodiscard]] VkQueue GetPresentQueue() const { return m_PresentQueue; }
    [[nodiscard]] VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    [[nodiscard]] VkCommandPool GetCommandPool() const { return m_CommandPool; }
    [[nodiscard]] VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
    [[nodiscard]] VmaAllocator GetAllocator() const { return m_Allocator; }

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
    void PickPhysicalDevice(vkb::Instance instance);

    void CreateLogicalDevice();
    void CreateCommandPool();

    /* Logical Device */
    vkb::Device m_Device;
    vkb::PhysicalDevice m_PhysicalDevice;

    VkSurfaceKHR m_Surface;

    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;

    VkCommandPool m_CommandPool;
    VkDescriptorPool m_DescriptorPool;

    VmaAllocator m_Allocator;

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    void CreateDescriptorPool();
};
