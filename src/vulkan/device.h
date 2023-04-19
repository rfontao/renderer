#pragma once

#include "../pch.h"

#include "utils.h"
#include "swapchain.h"

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Device {
public:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        [[nodiscard]] bool IsComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    Device(VkInstance instance, VkSurfaceKHR surface);
    ~Device();

    [[nodiscard]] SwapChainSupportDetails QuerySwapChainSupport() const;
    [[nodiscard]] QueueFamilyIndices FindQueueFamilies() const;
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    [[nodiscard]] VkDevice GetDevice() const { return m_Device; }
    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    [[nodiscard]] VkSurfaceKHR GetSurface() const { return m_Surface; }
    [[nodiscard]] VkQueue GetPresentQueue() const { return m_PresentQueue; }
    [[nodiscard]] VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    [[nodiscard]] VkCommandPool GetCommandPool() const { return m_CommandPool; }

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

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
};
