# glTF PBR Vulkan Renderer

A Physically-Based Renderer written in C++ using the Vulkan API.

Sponza with 2 point lights:
![sponza_point_lights](screenshots/sponza_point_lights.png)

Sponza with a directional light:
![sponza_point_lights](screenshots/sponza_directional_light.png)

Damaged helmet:
![damaged_helmet](screenshots/damaged_helmet.png)

## Features

- glTF 2.0 model loading, both .gltf and .glb formats, with tinygltf
- Physically-Based Rendering based on the glTF 2.0 specification. Supports:
  - Metallic-Roughness Textures
  - Normal textures (Normal mapping)
  - Emissive textures
  - Alpha mask
  - Multiple UV coordinates per vertex
- Skybox rendering
- Supports directional and point lights
- Two different camera movements
  - Arcball
  - Free movement
- Automatic Descriptor Set Layout creation based on shader reflection data with SPIRV-Reflect
- Dependency management with vcpkg
- Vulkan Dynamic Rendering
- Scene selector UI with Imgui

## TODO

- Shadows
- Fix max memory allocation (possibly with VMA)

## Dependencies

- Imgui (https://github.com/ocornut/imgui)
- stb_image (https://github.com/nothings/stb)
- vcpkg (https://github.com/microsoft/vcpkg)
- SPIRV-Reflect (https://github.com/KhronosGroup/SPIRV-Reflect)
- VulkanSDK (https://vulkan.lunarg.com/)
- glfw (https://github.com/glfw/glfw)
- glm (https://github.com/g-truc/glm)

## Relevant resources

- Vulkan Tutorial (https://vulkan-tutorial.com/)
- vkguide (https://vkguide.dev/)
- Sascha Willems' Vulkan Samples (https://github.com/SaschaWillems/Vulkan)
- Sascha Willems' Vulkan glTF PBR (https://github.com/SaschaWillems/Vulkan-glTF-PBR)
- learnopengl (https://learnopengl.com/)
- PBR in Filament (https://google.github.io/filament/Filament.html)
- VK_KHR_dynamic_rendering tutorial (https://lesleylai.info/en/vk-khr-dynamic-rendering/)
- TU Wien Vulkan Lecture Series (https://www.youtube.com/playlist?list=PLmIqTlJ6KsE1Jx5HV4sd2jOe3V1KMHHgn)
- Cubemaps (https://www.humus.name/index.php?page=Textures)
- glTF 2.0 spec (https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html)
- glTF sample models (https://github.com/KhronosGroup/glTF-Sample-Models)
- Vulkan Diagrams (https://github.com/David-DiGioia/vulkan-diagrams)
