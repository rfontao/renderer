#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "ExampleApplication.h"


void ExampleApplication::Run() {
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();

}

void ExampleApplication::InitVulkan() {

}

void ExampleApplication::MainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void ExampleApplication::Cleanup() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void ExampleApplication::InitWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Experimental Renderer", nullptr, nullptr);

}
