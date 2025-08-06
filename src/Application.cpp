#include "pch.h"

#include "Application.h"
#include "Vulkan/Utils.h"
#include "Vulkan/VulkanPipeline.h"

void Application::Run() {
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();
}

void Application::InitVulkan() {

    VK_CHECK(volkInitialize(), "Failed to initialize Volk");

    CreateInstance();

    FindScenePaths("models");

    VkSurfaceKHR surface = CreateSurface();
    m_Device = std::make_shared<VulkanDevice>(m_Instance, surface);
    m_Swapchain = std::make_shared<VulkanSwapchain>(m_Device, m_Window);

    debugDraw = std::make_unique<DebugDraw>(m_Device);

    VulkanPipeline::PipelineSpecification graphicsSpec{
            .vertShaderPath = "shaders/pbr.vert.spv",
            .fragShaderPath = "shaders/pbr_bindless.frag.spv",
    };
    m_GraphicsPipeline = std::make_shared<VulkanPipeline>(m_Device, graphicsSpec);

    VulkanPipeline::PipelineSpecification skyboxSpec{
            .vertShaderPath = "shaders/skybox.vert.spv",
            .fragShaderPath = "shaders/skybox.frag.spv",
            .cullingMode = VulkanPipeline::CullingMode::FRONT,
            .blendEnable = false,
            .enableDepthTesting = false,
    };
    m_SkyboxPipeline = std::make_shared<VulkanPipeline>(m_Device, skyboxSpec);

    VulkanPipeline::PipelineSpecification shadowMapSpec{
            .vertShaderPath = "shaders/shadowmap.vert.spv",
            .fragShaderPath = "shaders/shadowmap.frag.spv",
            .cullingMode = VulkanPipeline::CullingMode::NONE,
            .depthBiasEnable = true,
    };
    m_ShadowMapPipeline = std::make_shared<VulkanPipeline>(m_Device, shadowMapSpec);

    VulkanPipeline::PipelineSpecification debugDrawSpec{
            .vertShaderPath = "shaders/DebugDraw.vert.spv",
            .fragShaderPath = "shaders/DebugDraw.frag.spv",
            .cullingMode = VulkanPipeline::CullingMode::NONE,
            .wireframe = true,
    };
    debugDrawPipeline = std::make_shared<VulkanPipeline>(m_Device, debugDrawSpec);

    VulkanPipeline::PipelineSpecification frustumCullingSpec{
            .compShaderPath = "shaders/frustumCulling.comp.spv",
    };
    m_FrustumCullingPipeline = std::make_shared<VulkanPipeline>(m_Device, frustumCullingSpec);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap16.html#_cube_map_face_selection_and_transformations
    std::vector<std::filesystem::path> cubemapPaths = {
            "textures/cubemaps/vindelalven/posx.jpg", "textures/cubemaps/vindelalven/negx.jpg",
            "textures/cubemaps/vindelalven/posy.jpg", "textures/cubemaps/vindelalven/negy.jpg",
            "textures/cubemaps/vindelalven/posz.jpg", "textures/cubemaps/vindelalven/negz.jpg",
    };

    TextureSpecification cubemapTextureSpec{.name = "Skybox cubemap texture"};
    m_CubemapTexture = std::make_shared<TextureCube>(m_Device, cubemapTextureSpec, cubemapPaths);

    m_UI = UI(m_Device, m_Instance, m_Window, this);
    m_Scene = Scene(m_Device, m_ScenePaths[26], m_CubemapTexture, *debugDraw);

    TextureSpecification shadowmapTextureSpec{
            .name = "Shadow Depth Texture",
            .format = ImageFormat::D16,
            .width = shadowSize,
            .height = shadowSize,
    };
    m_ShadowDepthTexture = std::make_shared<Texture2D>(m_Device, shadowmapTextureSpec);

    CreateColorResources();
    CreateDepthResources();
    CreateBindlessTexturesArray();

    stagingManager.InitializeStagingBuffers(m_Device);

    stagingManager.AddCopy(m_Scene.m_Materials, m_Scene.materialsBuffer->GetBuffer());
    stagingManager.AddCopy(m_Scene.m_Lights, m_Scene.lightsBuffer->GetBuffer());

    m_Scene.UpdateCameraDatas();
    stagingManager.AddCopy(m_Scene.cameraDatas, m_Scene.camerasBuffer->GetBuffer());

    stagingManager.AddCopy(m_Scene.opaqueDrawIndirectCommands, m_Scene.opaqueDrawIndirectCommandsBuffer->GetBuffer());
    stagingManager.AddCopy(m_Scene.opaqueDrawData, m_Scene.opaqueDrawDataBuffer->GetBuffer());

    stagingManager.AddCopy(m_Scene.transparentDrawIndirectCommands,
                           m_Scene.transparentDrawIndirectCommandsBuffer->GetBuffer());
    stagingManager.AddCopy(m_Scene.transparentDrawData, m_Scene.transparentDrawDataBuffer->GetBuffer());

    stagingManager.AddCopy(m_Scene.globalModelMatrices, m_Scene.modelMatricesBuffer->GetBuffer());
}

void Application::MainLoop() {
    while (!glfwWindowShouldClose(m_Window)) {
        glfwPollEvents();
        HandleKeys();
        DrawFrame();
    }

    vkDeviceWaitIdle(m_Device->GetDevice());
}

void Application::Cleanup() {
    m_UI.Destroy();
    m_Swapchain->Destroy();

    m_ColorImage->Destroy();
    m_DepthImage->Destroy();

    m_CubemapTexture->Destroy();
    m_ShadowDepthTexture->Destroy();

    m_GraphicsPipeline->Destroy();
    m_SkyboxPipeline->Destroy();
    m_ShadowMapPipeline->Destroy();
    m_Scene.Destroy();
    m_Skybox.Destroy();

    stagingManager.Destroy();

    vkDestroySurfaceKHR(m_Instance, m_Device->GetSurface(), nullptr);
    m_Device->Destroy();

    vkb::destroy_instance(m_Instance);

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Application::InitWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_Window = glfwCreateWindow((int) WIDTH, (int) HEIGHT, "Experimental Renderer", nullptr, nullptr);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow *w, int width, int height) {
        const auto *app = static_cast<Application *>(glfwGetWindowUserPointer(w));
        app->m_Swapchain->m_NeedsResizing = true;
    });

    glfwSetMouseButtonCallback(m_Window, [](GLFWwindow *w, const int button, const int action, int mods) {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(w));

        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                app->m_Scene.cameras[app->cameraIndexControlling].SetMove(true);
                glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            } else if (action == GLFW_RELEASE) {
                app->m_Scene.cameras[app->cameraIndexControlling].SetMove(false);
                glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    });

    glfwSetCursorPosCallback(m_Window, [](GLFWwindow *w, const double xPosIn, const double yPosIn) {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(w));
        app->m_Scene.cameras[app->cameraIndexControlling].HandleMouseMovement(xPosIn, yPosIn);
    });

    glfwSetScrollCallback(m_Window, [](GLFWwindow *w, const double xScroll, const double yScroll) {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(w));
        app->m_Scene.cameras[app->cameraIndexControlling].HandleMouseScroll(yScroll);
    });

    glfwSetKeyCallback(m_Window, [](GLFWwindow *w, const int key, int scancode, const int action, int mods) {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(w));
        if (key == GLFW_KEY_TAB && action == GLFW_RELEASE) {
            app->cameraIndexControlling = (app->cameraIndexControlling + 1) % app->m_Scene.cameras.size();
        }
    });
}

void Application::HandleKeys() {

    auto &camera = m_Scene.cameras[cameraIndexControlling];

    if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
        camera.HandleMovement(Camera::MovementDirection::FRONT);
    if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
        camera.HandleMovement(Camera::MovementDirection::LEFT);
    if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
        camera.HandleMovement(Camera::MovementDirection::BACK);
    if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
        camera.HandleMovement(Camera::MovementDirection::RIGHT);
}

void Application::CreateInstance() {
    auto systemInfoRet = vkb::SystemInfo::get_system_info();
    if (!systemInfoRet) {
        throw std::runtime_error("Failed to get system info!");
    }
    const auto &systemInfo = systemInfoRet.value();

    vkb::InstanceBuilder instanceBuilder;
    instanceBuilder.set_app_name("Experimental Renderer")
            .set_engine_name("None")
            .require_api_version(1, 4, 0)
            .add_validation_feature_disable(VK_VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES_EXT)
            .enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // of course dedicated variable for validation
    if (enableValidationLayers && systemInfo.validation_layers_available) {
        instanceBuilder
                .enable_validation_layers()
                // .enable_layer("VK_LAYER_LUNARG_crash_diagnostic")
                // .enable_layer("VK_LAYER_LUNARG_api_dump")
                .use_default_debug_messenger();
    }

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        if (systemInfo.is_extension_available(extensions[i])) {
            instanceBuilder.enable_extension(extensions[i]);
        } else {
            throw std::runtime_error("Instance extension required by GLFW not available {}!");
        }
    }

    auto instanceRet = instanceBuilder.build();
    if (!instanceRet) {
        throw std::runtime_error("Failed to create vulkan instance!");
    }

    m_Instance = instanceRet.value();

    volkLoadInstance(m_Instance);
}

VkSurfaceKHR Application::CreateSurface() const {
    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &surface), "Failed to create window surface!");
    return surface;
}

void Application::CreateBindlessTexturesArray() {
    constexpr uint32_t maxBindlessArrayTextureSize = 1000;
    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxBindlessArrayTextureSize},
    };

    VkDescriptorPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
            .maxSets = 1000,
            .poolSizeCount = (uint32_t) poolSizes.size(),
            .pPoolSizes = poolSizes.data(),
    };

    VkDescriptorPool pool;
    VK_CHECK(vkCreateDescriptorPool(m_Device->GetDevice(), &poolInfo, nullptr, &pool),
             "Failed to create descriptor pool!");

    VkDescriptorSetLayoutBinding bindingLayoutInfo{};
    bindingLayoutInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindingLayoutInfo.descriptorCount = maxBindlessArrayTextureSize;
    bindingLayoutInfo.binding = 0;
    bindingLayoutInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindingLayoutInfo.pImmutableSamplers = nullptr;

    VkDescriptorBindingFlags bindingFlags =
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{};
    extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
    extendedInfo.pNext = nullptr;
    extendedInfo.bindingCount = 1;
    extendedInfo.pBindingFlags = &bindingFlags;

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.pNext = &extendedInfo;
    setLayoutInfo.bindingCount = 1;
    setLayoutInfo.pBindings = &bindingLayoutInfo;
    setLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

    VkDescriptorSetLayout layout;
    VK_CHECK(vkCreateDescriptorSetLayout(m_Device->GetDevice(), &setLayoutInfo, nullptr, &layout),
             "Failed to allocate bindless textures array set layout");

    VkDescriptorSetAllocateInfo allocationInfo{};
    allocationInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocationInfo.descriptorPool = pool;
    allocationInfo.descriptorSetCount = 1;
    allocationInfo.pSetLayouts = &layout;

    VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{};
    countInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    uint32_t max_binding = maxBindlessArrayTextureSize - 1;
    countInfo.descriptorSetCount = 1;
    countInfo.pDescriptorCounts = &max_binding; // This number is the max allocatable count

    allocationInfo.pNext = &countInfo;

    VK_CHECK(vkAllocateDescriptorSets(m_Device->GetDevice(), &allocationInfo, &m_BindlessTexturesSet),
             "Failed to allocate bindless textures array descriptor set");

    for (auto &tex: m_Scene.m_Textures) {
        m_TextureDescriptors.push_back({
                .sampler = m_Scene.m_Images[tex.imageIndex]->GetSampler(),
                .imageView = m_Scene.m_Images[tex.imageIndex]->GetImage()->GetImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        });
    }

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.dstBinding = 0;
    write.dstSet = m_BindlessTexturesSet;
    write.dstArrayElement = 0;
    write.descriptorCount = static_cast<uint32_t>(m_TextureDescriptors.size());
    write.pImageInfo = m_TextureDescriptors.data();

    vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &write, 0, nullptr);

    VkDescriptorImageInfo imageInfo{
            .sampler = m_CubemapTexture->GetSampler(),
            .imageView = m_CubemapTexture->GetImage()->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet write2{};
    write2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write2.dstBinding = 0;
    write2.dstSet = m_BindlessTexturesSet;
    write2.dstArrayElement = 750;
    write2.descriptorCount = 1;
    write2.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &write2, 0, nullptr);
}

void Application::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    stagingManager.Flush(commandBuffer);

    // Shadow rendering
    VkRenderingAttachmentInfo shadowDepthAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_ShadowDepthTexture->GetImage()->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    shadowDepthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo shadowRenderInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {0, 0, shadowSize, shadowSize},
            .layerCount = 1,
            .colorAttachmentCount = 0,
            .pDepthAttachment = &shadowDepthAttachment,
    };

    m_ShadowDepthTexture->GetImage()->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED,
                                                       VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    vkCmdBeginRendering(commandBuffer, &shadowRenderInfo);
    VkViewport shadowViewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) shadowSize,
            .height = (float) shadowSize,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &shadowViewport);

    VkRect2D shadowScissor{
            .offset = {0, 0},
            .extent = {shadowSize, shadowSize},
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &shadowScissor);

    vkCmdSetDepthBias(commandBuffer, shadowDepthBias, 0.0f, shadowDepthSlope);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowMapPipeline->GetPipeline());
    m_Scene.DrawShadowMap(commandBuffer, m_ShadowMapPipeline->GetLayout());

    vkCmdEndRendering(commandBuffer);

    m_ShadowDepthTexture->GetImage()->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // TODO: is this needed?
    // // Add barrier to prevent writing to commandbuffer until shadow map is done
    // VkMemoryBarrier2 shadowBarrier{
    //         .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
    //         .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
    //         .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    //         .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
    //         .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
    // };
    // VkDependencyInfo shadowDependencyInfo{
    //         .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    //         .memoryBarrierCount = 1,
    //         .pMemoryBarriers = &shadowBarrier,
    // };
    // vkCmdPipelineBarrier2(commandBuffer, &shadowDependencyInfo);

    struct FrustumCullingPushConstants {
        Frustum frustum;
        VkDeviceAddress commandBufferAddress;
        VkDeviceAddress drawDataAddress;
        VkDeviceAddress modelMatricesAddress;
    } frustumCullingPushConstants = {.frustum = m_Scene.cameras[0].GetFrustum(),
                                     .commandBufferAddress = m_Scene.opaqueDrawIndirectCommandsBuffer->GetAddress(),
                                     .drawDataAddress = m_Scene.opaqueDrawDataBuffer->GetAddress(),
                                     .modelMatricesAddress = m_Scene.modelMatricesBuffer->GetAddress()};

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_FrustumCullingPipeline->GetPipeline());
    vkCmdPushConstants(commandBuffer, m_FrustumCullingPipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(FrustumCullingPushConstants), &frustumCullingPushConstants);
    vkCmdDispatch(commandBuffer, (m_Scene.opaqueDrawIndirectCommands.size() + 255) / 256, 1, 1);

    // NOTE: This barrier is needed so that drawing only starts after the culling is performed
    // https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#upload-data-from-the-cpu-to-a-vertex-buffer
    VkMemoryBarrier2 cullingMemoryBarrier{.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                                          .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                          .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                                          .dstStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
                                          .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT};

    VkDependencyInfo cullingDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .memoryBarrierCount = 1,
            .pMemoryBarriers = &cullingMemoryBarrier,
    };

    vkCmdPipelineBarrier2(commandBuffer, &cullingDependencyInfo);

    VkDescriptorImageInfo imageInfo{
            .sampler = m_ShadowDepthTexture->GetSampler(),
            .imageView = m_ShadowDepthTexture->GetImage()->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.dstBinding = 0;
    write.dstSet = m_BindlessTexturesSet;
    write.dstArrayElement = 800;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &write, 0, nullptr);

    // Main scene render
    VkRenderingAttachmentInfo colorAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_Swapchain->GetImageView(imageIndex),
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = m_Swapchain->GetImageView(imageIndex),
            .resolveImageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    colorAttachment.clearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};

    VkRenderingAttachmentInfo depthAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_DepthImage->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo renderInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {0, 0, m_Swapchain->GetWidth(), m_Swapchain->GetHeight()},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
            .pDepthAttachment = &depthAttachment,
            //            .pStencilAttachment = &depthAttachment,
    };

    m_Swapchain->GetImage(imageIndex)
            ->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    m_DepthImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    vkCmdBeginRendering(commandBuffer, &renderInfo);
    VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) m_Swapchain->GetWidth(),
            .height = (float) m_Swapchain->GetHeight(),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{
            .offset = {0, 0},
            .extent = m_Swapchain->GetExtent(),
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Skybox
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyboxPipeline->GetPipeline());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyboxPipeline->GetLayout(), 0, 1,
                            &m_BindlessTexturesSet, 0, nullptr);

    m_Scene.DrawSkybox(commandBuffer, m_SkyboxPipeline->GetLayout());

    // Scene Rendering
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->GetPipeline());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->GetLayout(), 0, 1,
                            &m_BindlessTexturesSet, 0, nullptr);
    m_Scene.Draw(commandBuffer, m_GraphicsPipeline->GetLayout());
    m_UI.Draw(commandBuffer);

    vkCmdEndRendering(commandBuffer);

    // TODO: Evaluate if these are needed
    const VkImageMemoryBarrier2 depthBarrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                             .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                             .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                             .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                                             .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                             .image = m_DepthImage->GetImage(),
                                             .subresourceRange = {
                                                     .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                                                     .levelCount = 1,
                                                     .layerCount = 1,
                                             }};


    const VkImageMemoryBarrier2 graphicsBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .image = m_Swapchain->GetImage(imageIndex)->GetImage(),
            .subresourceRange =
                    {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .levelCount = 1,
                            .layerCount = 1,
                    },
    };


    std::array barriers = {depthBarrier, graphicsBarrier};
    VkDependencyInfo dependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = barriers.size(),
            .pImageMemoryBarriers = barriers.data(),
    };
    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

    // New rendering info
    VkRenderingAttachmentInfo colorAttachment2{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_Swapchain->GetImageView(imageIndex),
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = m_Swapchain->GetImageView(imageIndex),
            .resolveImageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    colorAttachment2.clearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};

    VkRenderingAttachmentInfo depthAttachment2{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_DepthImage->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    depthAttachment2.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo renderInfo2{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {0, 0, m_Swapchain->GetWidth(), m_Swapchain->GetHeight()},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment2,
            .pDepthAttachment = &depthAttachment2,
            //            .pStencilAttachment = &depthAttachment,
    };

    debugDraw->DrawAxis({0.0, 0.0, 0.0}, 1.0);
    // debugDraw->DrawFrustum(m_Scene.cameras[0].GetViewMatrix(), m_Scene.cameras[0].GetProjectionMatrix(),
    //                        {0.0, 0.0, 1.0});
    debugDraw->Draw(commandBuffer, stagingManager, *debugDrawPipeline, m_Scene, renderInfo2);

    m_Swapchain->GetImage(imageIndex)
            ->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer!");
}

void Application::DrawFrame() {

    vkWaitForFences(m_Device->GetDevice(), 1, &m_Swapchain->GetWaitFences()[m_CurrentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_Device->GetDevice(), 1, &m_Swapchain->GetWaitFences()[m_CurrentFrame]);

    const uint32_t imageIndex = m_Swapchain->AcquireNextImage(m_CurrentFrame);
    // Recreate swapchain
    if (imageIndex == std::numeric_limits<uint32_t>::max()) {
        m_Scene.cameras[cameraIndexDrawing].SetAspectRatio((double) m_Swapchain->GetWidth() /
                                                           (double) m_Swapchain->GetHeight());
        m_ColorImage->Destroy();
        CreateColorResources();
        m_DepthImage->Destroy();
        CreateDepthResources();
        return;
    }

    UpdateUniformBuffer(m_CurrentFrame);

    m_Scene.GenerateDrawCommands(*debugDraw, frustumCulling);

    stagingManager.AddCopy(m_Scene.opaqueDrawIndirectCommands, m_Scene.opaqueDrawIndirectCommandsBuffer->GetBuffer());
    stagingManager.AddCopy(m_Scene.opaqueDrawData, m_Scene.opaqueDrawDataBuffer->GetBuffer());

    stagingManager.AddCopy(m_Scene.transparentDrawIndirectCommands,
                           m_Scene.transparentDrawIndirectCommandsBuffer->GetBuffer());
    stagingManager.AddCopy(m_Scene.transparentDrawData, m_Scene.transparentDrawDataBuffer->GetBuffer());

    stagingManager.AddCopy(m_Scene.globalModelMatrices, m_Scene.modelMatricesBuffer->GetBuffer());

    vkResetCommandBuffer(m_Swapchain->GetCommandBuffers()[m_CurrentFrame], 0);
    RecordCommandBuffer(m_Swapchain->GetCommandBuffers()[m_CurrentFrame], imageIndex);

    VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    };

    VkSemaphore waitSemaphores[] = {m_Swapchain->GetImageAvailableSemaphores()[m_CurrentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_Swapchain->GetCommandBuffers()[m_CurrentFrame];

    VkSemaphore signalSemaphores[] = {m_Swapchain->GetRenderFinishedSemaphores()[m_CurrentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_CHECK(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_Swapchain->GetWaitFences()[m_CurrentFrame]),
             "Failed to submit draw command buffer!");

    bool resourceNeedResizing = m_Swapchain->Present(imageIndex, m_CurrentFrame);
    if (resourceNeedResizing) {
        m_Scene.cameras[cameraIndexDrawing].SetAspectRatio((double) m_Swapchain->GetWidth() /
                                                           (double) m_Swapchain->GetHeight());
        m_ColorImage->Destroy();
        CreateColorResources();
        m_DepthImage->Destroy();
        CreateDepthResources();
    }
    m_CurrentFrame = (m_CurrentFrame + 1) % m_Swapchain->numFramesInFlight;

    stagingManager.NextFrame();
    debugDraw->EndFrame();

    if (m_ShouldChangeScene)
        ChangeScene();
}

void Application::UpdateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    const auto currentTime = std::chrono::high_resolution_clock::now();
    const double time = std::chrono::duration<double>(currentTime - startTime).count();

    m_Scene.UpdateCameraDatas();
    stagingManager.AddCopy(m_Scene.cameraDatas, m_Scene.camerasBuffer->GetBuffer());

    // NOTE(RF): Directional light moving test
    m_Scene.m_Lights.at(0).direction.x = std::lerp(-0.8, 0.8, std::fmod(0.05 * time, 1.0));
    m_Scene.m_Lights.at(0).direction.z = std::lerp(-0.5, 0.5, std::fmod(0.05 * time, 1.0));
    m_Scene.m_Lights.at(0).view = glm::lookAt(m_Scene.m_Lights.at(0).direction * 15.0f, glm::vec3(0.0f, 0.0f, 0.0f),
                                              glm::vec3(0.0f, -1.0f, 0.0f)),
    stagingManager.AddCopy(m_Scene.m_Lights, m_Scene.lightsBuffer->GetBuffer());
}

void Application::CreateDepthResources() {
    ImageSpecification imageSpecification{
            .name = "Depth Image",
            .format = ImageFormat::D32,
            .usage = ImageUsage::Attachment,
            .width = m_Swapchain->GetWidth(),
            .height = m_Swapchain->GetHeight(),
            .mipLevels = 1,
            .layers = 1,
    };
    m_DepthImage = std::make_shared<VulkanImage>(m_Device, imageSpecification);
}

void Application::CreateColorResources() {
    ImageSpecification imageSpecification{
            .name = "Color Image",
            .format = ImageFormat::R8G8B8A8_SRGB,
            .usage = ImageUsage::Attachment,
            .width = m_Swapchain->GetWidth(),
            .height = m_Swapchain->GetHeight(),
            .mipLevels = 1,
            .layers = 1,
    };
    m_ColorImage = std::make_shared<VulkanImage>(m_Device, imageSpecification);
}

void Application::SetScene(const std::filesystem::path &scenePath) {
    m_ShouldChangeScene = true;
    m_NextScenePath = scenePath;
}

void Application::ChangeScene() {
    m_ShouldChangeScene = false;
    vkDeviceWaitIdle(m_Device->GetDevice());
    m_Scene.Destroy();
    m_Scene = Scene(m_Device, m_NextScenePath, m_CubemapTexture, *debugDraw);

    m_TextureDescriptors.clear();
    CreateBindlessTexturesArray();

    stagingManager.AddCopy(m_Scene.m_Materials, m_Scene.materialsBuffer->GetBuffer());
    stagingManager.AddCopy(m_Scene.m_Lights, m_Scene.lightsBuffer->GetBuffer());

    stagingManager.AddCopy(m_Scene.opaqueDrawIndirectCommands, m_Scene.opaqueDrawIndirectCommandsBuffer->GetBuffer());
    stagingManager.AddCopy(m_Scene.opaqueDrawData, m_Scene.opaqueDrawDataBuffer->GetBuffer());

    stagingManager.AddCopy(m_Scene.transparentDrawIndirectCommands,
                           m_Scene.transparentDrawIndirectCommandsBuffer->GetBuffer());
    stagingManager.AddCopy(m_Scene.transparentDrawData, m_Scene.transparentDrawDataBuffer->GetBuffer());

    stagingManager.AddCopy(m_Scene.globalModelMatrices, m_Scene.modelMatricesBuffer->GetBuffer());
}

void Application::FindScenePaths(const std::filesystem::path &basePath) {
    for (const auto &dirEntry: std::filesystem::recursive_directory_iterator(basePath)) {
        if (dirEntry.is_regular_file() &&
            (dirEntry.path().extension() == ".gltf" || dirEntry.path().extension() == ".glb")) {
            m_ScenePaths.push_back(dirEntry.path());
        }
    }
}
