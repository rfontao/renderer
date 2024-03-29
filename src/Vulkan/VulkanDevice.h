#pragma once
#include "../pch.h"

#include "Utils.h"
#include "VulkanSwapchain.h"

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanDevice {
public:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        [[nodiscard]] bool IsComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
    void Destroy();

    [[nodiscard]] SwapChainSupportDetails QuerySwapChainSupport() const;
    [[nodiscard]] QueueFamilyIndices FindQueueFamilies() const;
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    [[nodiscard]] VkDevice GetDevice() const { return m_Device; }
    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    [[nodiscard]] VkSurfaceKHR GetSurface() const { return m_Surface; }
    [[nodiscard]] VkQueue GetPresentQueue() const { return m_PresentQueue; }
    [[nodiscard]] VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    [[nodiscard]] VkCommandPool GetCommandPool() const { return m_CommandPool; }
    [[nodiscard]] VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
    [[nodiscard]] VmaAllocator GetAllocator() const { return m_Allocator; }
//    [[nodiscard]] QueueFamilyIndices GetFamilyIndices() const { return m_QueueFamilyProperties; }

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

    VkFormat FindDepthFormat();
    VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                        VkFormatFeatureFlags features);

private:
    [[nodiscard]] VkSampleCountFlagBits GetMaxUsableSampleCount() const;
    [[nodiscard]] bool CheckDeviceExtensionSupport(const std::vector<const char *> &extensions) const;
    bool IsDeviceSuitable();
    void PickPhysicalDevice(VkInstance instance);

    void CreateLogicalDevice();
    void CreateCommandPool();

    /* Logical Device */
    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;

    VkSurfaceKHR m_Surface;

    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;

    // Device properties
    VkPhysicalDeviceProperties m_DeviceProperties;
    VkPhysicalDeviceFeatures m_Features;
    VkPhysicalDeviceFeatures m_EnabledFeatures;
    VkPhysicalDeviceMemoryProperties m_MemoryProperties;
    std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
    std::vector<std::string> m_SupportedExtensions;

    VkCommandPool m_CommandPool;
    VkDescriptorPool m_DescriptorPool;

    VmaAllocator m_Allocator;

    const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    const std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    void CreateDescriptorPool();
};
