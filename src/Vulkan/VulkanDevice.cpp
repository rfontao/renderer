#include "VulkanDevice.h"
#include "VkBootstrap.h"
#include "pch.h"

VulkanDevice::VulkanDevice(vkb::Instance instance, VkSurfaceKHR surface) : m_Surface(surface) {
    PickPhysicalDevice(instance);

    CreateLogicalDevice();
    CreateCommandPool();
    CreateDescriptorPool();

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
    allocatorCreateInfo.device = m_Device;
    allocatorCreateInfo.instance = instance;
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    //    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
    vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator);
}

void VulkanDevice::Destroy() {
    vmaDestroyAllocator(m_Allocator);
    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    vkb::destroy_device(m_Device);
}

void VulkanDevice::PickPhysicalDevice(vkb::Instance instance) {
    VkPhysicalDeviceFeatures deviceFeatures{
            .multiDrawIndirect = VK_TRUE,
            .samplerAnisotropy = VK_TRUE,
    };

    VkPhysicalDeviceVulkan11Features vulkan11Features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,

            // Shader Draw Parameters
            .shaderDrawParameters = VK_TRUE,
    };

    VkPhysicalDeviceVulkan12Features vulkan12Features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,

            // Draw indirect count
            .drawIndirectCount = VK_TRUE,

            // Descriptor Indexing
            .descriptorIndexing = VK_TRUE,
            .shaderUniformBufferArrayNonUniformIndexing = VK_TRUE,
            .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
            .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
            .descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
            .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
            .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
            .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
            .descriptorBindingPartiallyBound = VK_TRUE,
            .runtimeDescriptorArray = VK_TRUE,

            // Scalar Block Layout
            .scalarBlockLayout = VK_TRUE,

            // Buffer Device Address
            .bufferDeviceAddress = VK_TRUE,
    };

    VkPhysicalDeviceVulkan13Features vulkan13Features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,

            // Synchronization 2
            .synchronization2 = VK_TRUE,

            // Dynamic Rendering
            .dynamicRendering = VK_TRUE,
    };

    vkb::PhysicalDeviceSelector physicalDeviceSelector(instance);
    auto physicalDeviceSelectorReturn = physicalDeviceSelector.set_surface(m_Surface)
                                                .add_required_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
                                                .set_required_features(deviceFeatures)
                                                .set_required_features_11(vulkan11Features)
                                                .set_required_features_12(vulkan12Features)
                                                .set_required_features_13(vulkan13Features)
                                                .require_present()
                                                .select();
    if (!physicalDeviceSelectorReturn) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    m_PhysicalDevice = physicalDeviceSelectorReturn.value();
}

void VulkanDevice::CreateLogicalDevice() {
    vkb::DeviceBuilder deviceBuilder{m_PhysicalDevice};
    auto deviceRet = deviceBuilder.build();
    if (!deviceRet) {
        throw std::runtime_error("Failed to create logical device!");
    }
    m_Device = deviceRet.value();

    m_GraphicsQueue = m_Device.get_queue(vkb::QueueType::graphics).value();
    m_ComputeQueue = m_Device.get_queue(vkb::QueueType::compute).value();
    m_PresentQueue = m_Device.get_queue(vkb::QueueType::present).value();
}

void VulkanDevice::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_Device.get_queue_index(vkb::QueueType::graphics).value(),
    };

    VK_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool), "Failed to create command pool!");
}

void VulkanDevice::CreateDescriptorPool() {
    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
    };

    VkDescriptorPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1000,
            .poolSizeCount = (uint32_t) poolSizes.size(),
            .pPoolSizes = poolSizes.data(),
    };

    VK_CHECK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool),
             "Failed to create descriptor pool!");
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
