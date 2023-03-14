cmake_minimum_required(VERSION 3.24)
project(renderer)

set(CMAKE_CXX_STANDARD 17)

# https://vulkan.lunarg.com/doc/sdk/1.3.239.0/windows/layer_configuration.html
set(ENV{VK_LOADER_LAYERS_ENABLE} *api_dump,*validation)

# Vulkan
find_package(Vulkan REQUIRED)

add_subdirectory(external/glfw-3.3.8)
add_subdirectory(external/glm)

add_executable(renderer src/main.cpp src/ExampleApplication.cpp src/ExampleApplication.h)
target_link_libraries(${PROJECT_NAME} glfw Vulkan::Vulkan glm)
