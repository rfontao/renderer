set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
set(VCPKG_DEFAULT_TRIPLET "x64-windows")

add_compile_definitions(TINYGLTF_ENABLE_DRACO)

cmake_minimum_required(VERSION 3.24)
project(renderer)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Stb REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(draco CONFIG REQUIRED)

add_subdirectory(external/vk-bootstrap)

file(GLOB_RECURSE SourceFiles "src/*.cpp")
add_executable(renderer ${SourceFiles})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)

target_link_libraries(${PROJECT_NAME}
        glfw
        Vulkan::Vulkan
        glm::glm
        unofficial::spirv-reflect
        imgui::imgui
        GPUOpen::VulkanMemoryAllocator
        draco::draco
        vk-bootstrap::vk-bootstrap
)

target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${Spirv_reflect_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${TINYGLTF_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE src)
