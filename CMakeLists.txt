cmake_minimum_required(VERSION 3.24)
project(renderer)

set(CMAKE_CXX_STANDARD 17)

# Vulkan
find_package(Vulkan REQUIRED)

add_subdirectory(external/glfw-3.3.8)
add_subdirectory(external/glm)

add_executable(renderer src/main.cpp src/ExampleApplication.cpp src/ExampleApplication.h)
target_link_libraries(${PROJECT_NAME} glfw Vulkan::Vulkan glm)
