set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")

cmake_minimum_required(VERSION 3.24)
project(renderer)

set(CMAKE_CXX_STANDARD 17)

# Vulkan
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(renderer src/main.cpp src/ExampleApplication.cpp src/ExampleApplication.h)
target_link_libraries(${PROJECT_NAME} glfw Vulkan::Vulkan glm::glm)
