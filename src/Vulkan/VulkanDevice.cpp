#include "pch.h"
#include "VulkanDevice.h"

#include "VulkanSwapchain.h"

VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface) : m_Surface(surface) {
    PickPhysicalDevice(instance);

    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
    vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_Features);
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
    m_QueueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, m_QueueFamilyProperties.data());

    // Get list of supported extensions
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, nullptr);
    if (extCount > 0) {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, &extensions.front()) ==
            VK_SUCCESS) {
            for (auto ext: extensions) {
                m_SupportedExtensions.emplace_back(ext.extensionName);
            }
        }
    }

    CreateLogicalDevice();
    CreateCommandPool();
}

void VulkanDevice::Destroy() {
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    vkDestroyDevice(m_Device, nullptr);
}

void VulkanDevice::PickPhysicalDevice(VkInstance instance) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    bool foundSuitableDevice = false;
    for (const auto &d: devices) {
        m_PhysicalDevice = d;
        if (IsDeviceSuitable()) {
//            msaaSamples = GetMaxUsableSampleCount();
            foundSuitableDevice = true;
            break;
        }
    }

    if (!foundSuitableDevice) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

void VulkanDevice::CreateLogicalDevice() {
    QueueFamilyIndices indices = FindQueueFamilies();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily: uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamily,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority
        };
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{
            .samplerAnisotropy = VK_TRUE,
    };

    constexpr VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
            .dynamicRendering = VK_TRUE,
    };

    VkDeviceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &dynamicRenderingFeature,
            .queueCreateInfoCount = (uint32_t) queueCreateInfos.size(),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledExtensionCount = (uint32_t) (deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
            .pEnabledFeatures = &deviceFeatures,
    };

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VK_CHECK(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device), "Failed to create logical device!");

    vkGetDeviceQueue(m_Device, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, indices.presentFamily.value(), 0, &m_PresentQueue);
}

void VulkanDevice::CreateCommandPool() {
    VulkanDevice::QueueFamilyIndices queueFamilyIndices = FindQueueFamilies();

    VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
    };

    VK_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool),
             "Failed to create command pool!");
}


SwapChainSupportDetails VulkanDevice::QuerySwapChainSupport() const {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount,
                                             details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount,
                                                  details.presentModes.data());
    }

    return details;
}

bool VulkanDevice::IsDeviceSuitable() {
    QueueFamilyIndices indices = FindQueueFamilies();
    bool extensionsSupported = CheckDeviceExtensionSupport(deviceExtensions);

    bool swapChainAdequate;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport();
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &supportedFeatures);

    return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

// Check if the device supports extensions
bool VulkanDevice::CheckDeviceExtensionSupport(const std::vector<const char *> &extensions) const {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

    for (const auto &extension: availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

VulkanDevice::QueueFamilyIndices VulkanDevice::FindQueueFamilies() const {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily: queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.IsComplete()) {
            break;
        }
        i++;
    }

    return indices;
}

VkSampleCountFlagBits VulkanDevice::GetMaxUsableSampleCount() const {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (m_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

VkCommandBuffer VulkanDevice::BeginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void VulkanDevice::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
    };

    vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_GraphicsQueue);

    vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
}


