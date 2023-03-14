#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

class ExampleApplication {
public:
    void Run();

private:
    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void Cleanup();
    void CreateInstance();
    void PrintAvailableVulkanExtensions();
    bool CheckValidationLayerSupport();

    VkInstance instance;

    GLFWwindow* window;

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

};
