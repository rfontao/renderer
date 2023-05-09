#pragma once

#include <Vulkan/VulkanDevice.h>

class UI {
public:

    UI() = default;
    UI(std::shared_ptr<VulkanDevice> device, VkInstance instance, GLFWwindow *window);
    void Destroy();

    void Draw(VkCommandBuffer cmd);
private:

    VkDescriptorPool m_DescriptorPool;
    std::shared_ptr<VulkanDevice> m_Device;
};

