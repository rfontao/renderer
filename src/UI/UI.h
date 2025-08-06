#pragma once

#include <Vulkan/VulkanDevice.h>

class Application;

class UI {
public:

    UI() = default;

    UI(std::shared_ptr<VulkanDevice> device, VkInstance instance, GLFWwindow *window, Application *app);

    void Destroy();

    void Draw(VkCommandBuffer cmd);

private:

    Application *app{nullptr};
    VkDescriptorPool descriptorPool{};
    std::shared_ptr<VulkanDevice> device;
};

