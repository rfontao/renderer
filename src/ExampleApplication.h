#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class ExampleApplication {
public:
    void Run();

private:
    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void Cleanup();

    GLFWwindow* window;

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
};
