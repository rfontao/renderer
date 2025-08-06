#pragma once
#include "../pch.h"

#include "VkBootstrap.h"

class VulkanDevice {
public:
    VulkanDevice(vkb::Instance instance, VkSurfaceKHR surface);
    void Destroy();

    [[nodiscard]] vkb::Device GetDevice() const { return device; }
    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
    [[nodiscard]] VkSurfaceKHR GetSurface() const { return surface; }
    [[nodiscard]] VkQueue GetPresentQueue() const { return presentQueue; }
    [[nodiscard]] VkQueue GetGraphicsQueue() const { return graphicsQueue; }
    [[nodiscard]] VkCommandPool GetCommandPool() const { return commandPool; }
    [[nodiscard]] VkDescriptorPool GetDescriptorPool() const { return descriptorPool; }
    [[nodiscard]] VmaAllocator GetAllocator() const { return allocator; }

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
    void PickPhysicalDevice(vkb::Instance instance);

    void CreateLogicalDevice();
    void CreateCommandPool();

    /* Logical Device */
    vkb::Device device;
    vkb::PhysicalDevice physicalDevice;

    VkSurfaceKHR surface;

    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue presentQueue;

    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;

    VmaAllocator allocator;

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    void CreateDescriptorPool();
};
