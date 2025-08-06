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
    device = std::make_shared<VulkanDevice>(instance, surface);
    swapchain = std::make_shared<VulkanSwapchain>(device, window);

    debugDraw = std::make_unique<DebugDraw>(device);

    VulkanPipeline::PipelineSpecification graphicsSpec{
            .vertShaderPath = "shaders/pbr.vert.spv",
            .fragShaderPath = "shaders/pbr_bindless.frag.spv",
    };
    graphicsPipeline = std::make_shared<VulkanPipeline>(device, graphicsSpec);

    VulkanPipeline::PipelineSpecification skyboxSpec{
            .vertShaderPath = "shaders/skybox.vert.spv",
            .fragShaderPath = "shaders/skybox.frag.spv",
            .cullingMode = VulkanPipeline::CullingMode::FRONT,
            .blendEnable = false,
            .enableDepthTesting = false,
    };
    skyboxPipeline = std::make_shared<VulkanPipeline>(device, skyboxSpec);

    VulkanPipeline::PipelineSpecification shadowMapSpec{
            .vertShaderPath = "shaders/shadowmap.vert.spv",
            .fragShaderPath = "shaders/shadowmap.frag.spv",
            .cullingMode = VulkanPipeline::CullingMode::NONE,
            .depthBiasEnable = true,
    };
    shadowMapPipeline = std::make_shared<VulkanPipeline>(device, shadowMapSpec);

    VulkanPipeline::PipelineSpecification debugDrawSpec{
            .vertShaderPath = "shaders/DebugDraw.vert.spv",
            .fragShaderPath = "shaders/DebugDraw.frag.spv",
            .cullingMode = VulkanPipeline::CullingMode::NONE,
            .wireframe = true,
    };
    debugDrawPipeline = std::make_shared<VulkanPipeline>(device, debugDrawSpec);

    VulkanPipeline::PipelineSpecification frustumCullingSpec{
            .compShaderPath = "shaders/frustumCulling.comp.spv",
    };
    frustumCullingPipeline = std::make_shared<VulkanPipeline>(device, frustumCullingSpec);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap16.html#_cube_map_face_selection_and_transformations
    std::vector<std::filesystem::path> cubemapPaths = {
            "textures/cubemaps/vindelalven/posx.jpg", "textures/cubemaps/vindelalven/negx.jpg",
            "textures/cubemaps/vindelalven/posy.jpg", "textures/cubemaps/vindelalven/negy.jpg",
            "textures/cubemaps/vindelalven/posz.jpg", "textures/cubemaps/vindelalven/negz.jpg",
    };

    TextureSpecification cubemapTextureSpec{.name = "Skybox cubemap texture"};
    cubemapTexture = std::make_shared<TextureCube>(device, cubemapTextureSpec, cubemapPaths);

    userInterface = UI(device, instance, window, this);
    scene = std::make_unique<Scene>(device, scenePaths[26], cubemapTexture, *debugDraw);

    TextureSpecification shadowmapTextureSpec{
            .name = "Shadow Depth Texture",
            .format = ImageFormat::D16,
            .width = shadowSize,
            .height = shadowSize,
    };
    shadowDepthTexture = std::make_shared<Texture2D>(device, shadowmapTextureSpec);

    CreateColorResources();
    CreateDepthResources();
    CreateBindlessTexturesArray();

    stagingManager.InitializeStagingBuffers(device);

    stagingManager.AddCopy(scene->materials, scene->materialsBuffer->GetBuffer());
    stagingManager.AddCopy(scene->lights, scene->lightsBuffer->GetBuffer());

    scene->UpdateCameraDatas();
    stagingManager.AddCopy(scene->cameraDatas, scene->camerasBuffer->GetBuffer());

    stagingManager.AddCopy(scene->opaqueDrawIndirectCommands, scene->opaqueDrawIndirectCommandsBuffer->GetBuffer());
    stagingManager.AddCopy(scene->opaqueDrawData, scene->opaqueDrawDataBuffer->GetBuffer());

    stagingManager.AddCopy(scene->transparentDrawIndirectCommands,
                           scene->transparentDrawIndirectCommandsBuffer->GetBuffer());
    stagingManager.AddCopy(scene->transparentDrawData, scene->transparentDrawDataBuffer->GetBuffer());

    stagingManager.AddCopy(scene->globalModelMatrices, scene->modelMatricesBuffer->GetBuffer());
}

void Application::MainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        HandleKeys();
        DrawFrame();
    }

    vkDeviceWaitIdle(device->GetDevice());
}

void Application::Cleanup() {
    userInterface.Destroy();
    swapchain->Destroy();

    colorImage->Destroy();
    depthImage->Destroy();

    cubemapTexture->Destroy();
    shadowDepthTexture->Destroy();

    graphicsPipeline->Destroy();
    skyboxPipeline->Destroy();
    shadowMapPipeline->Destroy();
    scene->Destroy();
    skybox->Destroy();

    stagingManager.Destroy();

    vkDestroySurfaceKHR(instance, device->GetSurface(), nullptr);
    device->Destroy();

    vkb::destroy_instance(instance);

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::InitWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow((int) WIDTH, (int) HEIGHT, "Experimental Renderer", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *w, int width, int height) {
        const auto *app = static_cast<Application *>(glfwGetWindowUserPointer(w));
        app->swapchain->needsResizing = true;
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow *w, const int button, const int action, int mods) {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(w));

        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                app->scene->cameras[app->cameraIndexControlling].SetMove(true);
                glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            } else if (action == GLFW_RELEASE) {
                app->scene->cameras[app->cameraIndexControlling].SetMove(false);
                glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow *w, const double xPosIn, const double yPosIn) {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(w));
        app->scene->cameras[app->cameraIndexControlling].HandleMouseMovement(xPosIn, yPosIn);
    });

    glfwSetScrollCallback(window, [](GLFWwindow *w, const double xScroll, const double yScroll) {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(w));
        app->scene->cameras[app->cameraIndexControlling].HandleMouseScroll(yScroll);
    });

    glfwSetKeyCallback(window, [](GLFWwindow *w, const int key, int scancode, const int action, int mods) {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(w));
        if (key == GLFW_KEY_TAB && action == GLFW_RELEASE) {
            app->cameraIndexControlling = (app->cameraIndexControlling + 1) % app->scene->cameras.size();
        }
    });
}

void Application::HandleKeys() {

    auto &camera = scene->cameras[cameraIndexControlling];

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.HandleMovement(Camera::MovementDirection::FRONT);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.HandleMovement(Camera::MovementDirection::LEFT);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.HandleMovement(Camera::MovementDirection::BACK);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
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

    instance = instanceRet.value();

    volkLoadInstance(instance);
}

VkSurfaceKHR Application::CreateSurface() const {
    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface), "Failed to create window surface!");
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
    VK_CHECK(vkCreateDescriptorPool(device->GetDevice(), &poolInfo, nullptr, &pool),
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
    VK_CHECK(vkCreateDescriptorSetLayout(device->GetDevice(), &setLayoutInfo, nullptr, &layout),
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

    VK_CHECK(vkAllocateDescriptorSets(device->GetDevice(), &allocationInfo, &bindlessTexturesSet),
             "Failed to allocate bindless textures array descriptor set");

    for (auto &tex: scene->textures) {
        textureDescriptors.push_back({
                .sampler = scene->images[tex.imageIndex]->GetSampler(),
                .imageView = scene->images[tex.imageIndex]->GetImage()->GetImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        });
    }

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.dstBinding = 0;
    write.dstSet = bindlessTexturesSet;
    write.dstArrayElement = 0;
    write.descriptorCount = static_cast<uint32_t>(textureDescriptors.size());
    write.pImageInfo = textureDescriptors.data();

    vkUpdateDescriptorSets(device->GetDevice(), 1, &write, 0, nullptr);

    VkDescriptorImageInfo imageInfo{
            .sampler = cubemapTexture->GetSampler(),
            .imageView = cubemapTexture->GetImage()->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet write2{};
    write2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write2.dstBinding = 0;
    write2.dstSet = bindlessTexturesSet;
    write2.dstArrayElement = 750;
    write2.descriptorCount = 1;
    write2.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device->GetDevice(), 1, &write2, 0, nullptr);
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
            .imageView = shadowDepthTexture->GetImage()->GetImageView(),
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

    shadowDepthTexture->GetImage()->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED,
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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipeline->GetPipeline());
    scene->DrawShadowMap(commandBuffer, shadowMapPipeline->GetLayout());

    vkCmdEndRendering(commandBuffer);

    shadowDepthTexture->GetImage()->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
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
    } frustumCullingPushConstants = {.frustum = scene->cameras[0].GetFrustum(),
                                     .commandBufferAddress = scene->opaqueDrawIndirectCommandsBuffer->GetAddress(),
                                     .drawDataAddress = scene->opaqueDrawDataBuffer->GetAddress(),
                                     .modelMatricesAddress = scene->modelMatricesBuffer->GetAddress()};

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, frustumCullingPipeline->GetPipeline());
    vkCmdPushConstants(commandBuffer, frustumCullingPipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(FrustumCullingPushConstants), &frustumCullingPushConstants);
    vkCmdDispatch(commandBuffer, (scene->opaqueDrawIndirectCommands.size() + 255) / 256, 1, 1);

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
            .sampler = shadowDepthTexture->GetSampler(),
            .imageView = shadowDepthTexture->GetImage()->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.dstBinding = 0;
    write.dstSet = bindlessTexturesSet;
    write.dstArrayElement = 800;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device->GetDevice(), 1, &write, 0, nullptr);

    // Main scene render
    VkRenderingAttachmentInfo colorAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchain->GetImageView(imageIndex),
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = swapchain->GetImageView(imageIndex),
            .resolveImageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    colorAttachment.clearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};

    VkRenderingAttachmentInfo depthAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = depthImage->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo renderInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {0, 0, swapchain->GetWidth(), swapchain->GetHeight()},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
            .pDepthAttachment = &depthAttachment,
            //            .pStencilAttachment = &depthAttachment,
    };

    swapchain->GetImage(imageIndex)
            ->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    depthImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    vkCmdBeginRendering(commandBuffer, &renderInfo);
    VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) swapchain->GetWidth(),
            .height = (float) swapchain->GetHeight(),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{
            .offset = {0, 0},
            .extent = swapchain->GetExtent(),
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Skybox
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline->GetPipeline());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline->GetLayout(), 0, 1,
                            &bindlessTexturesSet, 0, nullptr);

    scene->DrawSkybox(commandBuffer, skyboxPipeline->GetLayout());

    // Scene Rendering
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetPipeline());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetLayout(), 0, 1,
                            &bindlessTexturesSet, 0, nullptr);
    scene->Draw(commandBuffer, graphicsPipeline->GetLayout());
    userInterface.Draw(commandBuffer);

    vkCmdEndRendering(commandBuffer);

    // TODO: Evaluate if these are needed
    const VkImageMemoryBarrier2 depthBarrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                             .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                             .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                             .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                                             .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                             .image = depthImage->GetImage(),
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
            .image = swapchain->GetImage(imageIndex)->GetImage(),
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
            .imageView = swapchain->GetImageView(imageIndex),
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = swapchain->GetImageView(imageIndex),
            .resolveImageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    colorAttachment2.clearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};

    VkRenderingAttachmentInfo depthAttachment2{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = depthImage->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    depthAttachment2.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo renderInfo2{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {0, 0, swapchain->GetWidth(), swapchain->GetHeight()},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment2,
            .pDepthAttachment = &depthAttachment2,
            //            .pStencilAttachment = &depthAttachment,
    };

    debugDraw->DrawAxis({0.0, 0.0, 0.0}, 1.0);
    // debugDraw->DrawFrustum(m_Scene.cameras[0].GetViewMatrix(), m_Scene.cameras[0].GetProjectionMatrix(),
    //                        {0.0, 0.0, 1.0});
    debugDraw->Draw(commandBuffer, stagingManager, *debugDrawPipeline, *scene, renderInfo2);

    swapchain->GetImage(imageIndex)
            ->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer!");
}

void Application::DrawFrame() {

    vkWaitForFences(device->GetDevice(), 1, &swapchain->GetWaitFences()[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device->GetDevice(), 1, &swapchain->GetWaitFences()[currentFrame]);

    const uint32_t imageIndex = swapchain->AcquireNextImage(currentFrame);
    // Recreate swapchain
    if (imageIndex == std::numeric_limits<uint32_t>::max()) {
        scene->cameras[cameraIndexDrawing].SetAspectRatio((double) swapchain->GetWidth() /
                                                           (double) swapchain->GetHeight());
        colorImage->Destroy();
        CreateColorResources();
        depthImage->Destroy();
        CreateDepthResources();
        return;
    }

    UpdateUniformBuffer(currentFrame);

    scene->GenerateDrawCommands(*debugDraw, frustumCulling);

    stagingManager.AddCopy(scene->opaqueDrawIndirectCommands, scene->opaqueDrawIndirectCommandsBuffer->GetBuffer());
    stagingManager.AddCopy(scene->opaqueDrawData, scene->opaqueDrawDataBuffer->GetBuffer());

    stagingManager.AddCopy(scene->transparentDrawIndirectCommands,
                           scene->transparentDrawIndirectCommandsBuffer->GetBuffer());
    stagingManager.AddCopy(scene->transparentDrawData, scene->transparentDrawDataBuffer->GetBuffer());

    stagingManager.AddCopy(scene->globalModelMatrices, scene->modelMatricesBuffer->GetBuffer());

    vkResetCommandBuffer(swapchain->GetCommandBuffers()[currentFrame], 0);
    RecordCommandBuffer(swapchain->GetCommandBuffers()[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    };

    VkSemaphore waitSemaphores[] = {swapchain->GetImageAvailableSemaphores()[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &swapchain->GetCommandBuffers()[currentFrame];

    VkSemaphore signalSemaphores[] = {swapchain->GetRenderFinishedSemaphores()[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_CHECK(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, swapchain->GetWaitFences()[currentFrame]),
             "Failed to submit draw command buffer!");

    bool resourceNeedResizing = swapchain->Present(imageIndex, currentFrame);
    if (resourceNeedResizing) {
        scene->cameras[cameraIndexDrawing].SetAspectRatio((double) swapchain->GetWidth() /
                                                           (double) swapchain->GetHeight());
        colorImage->Destroy();
        CreateColorResources();
        depthImage->Destroy();
        CreateDepthResources();
    }
    currentFrame = (currentFrame + 1) % swapchain->numFramesInFlight;

    stagingManager.NextFrame();
    debugDraw->EndFrame();

    if (shouldChangeScene)
        ChangeScene();
}

void Application::UpdateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    const auto currentTime = std::chrono::high_resolution_clock::now();
    const double time = std::chrono::duration<double>(currentTime - startTime).count();

    scene->UpdateCameraDatas();
    stagingManager.AddCopy(scene->cameraDatas, scene->camerasBuffer->GetBuffer());

    // NOTE(RF): Directional light moving test
    scene->lights.at(0).direction.x = std::lerp(-0.8, 0.8, std::fmod(0.05 * time, 1.0));
    scene->lights.at(0).direction.z = std::lerp(-0.5, 0.5, std::fmod(0.05 * time, 1.0));
    scene->lights.at(0).view = glm::lookAt(scene->lights.at(0).direction * 15.0f, glm::vec3(0.0f, 0.0f, 0.0f),
                                              glm::vec3(0.0f, -1.0f, 0.0f)),
    stagingManager.AddCopy(scene->lights, scene->lightsBuffer->GetBuffer());
}

void Application::CreateDepthResources() {
    ImageSpecification imageSpecification{
            .name = "Depth Image",
            .format = ImageFormat::D32,
            .usage = ImageUsage::Attachment,
            .width = swapchain->GetWidth(),
            .height = swapchain->GetHeight(),
            .mipLevels = 1,
            .layers = 1,
    };
    depthImage = std::make_shared<VulkanImage>(device, imageSpecification);
}

void Application::CreateColorResources() {
    ImageSpecification imageSpecification{
            .name = "Color Image",
            .format = ImageFormat::R8G8B8A8_SRGB,
            .usage = ImageUsage::Attachment,
            .width = swapchain->GetWidth(),
            .height = swapchain->GetHeight(),
            .mipLevels = 1,
            .layers = 1,
    };
    colorImage = std::make_shared<VulkanImage>(device, imageSpecification);
}

void Application::SetScene(const std::filesystem::path &scenePath) {
    shouldChangeScene = true;
    nextScenePath = scenePath;
}

void Application::ChangeScene() {
    shouldChangeScene = false;
    vkDeviceWaitIdle(device->GetDevice());
    scene->Destroy();
    scene = std::make_unique<Scene>(device, nextScenePath, cubemapTexture, *debugDraw);

    textureDescriptors.clear();
    CreateBindlessTexturesArray();

    stagingManager.AddCopy(scene->materials, scene->materialsBuffer->GetBuffer());
    stagingManager.AddCopy(scene->lights, scene->lightsBuffer->GetBuffer());

    stagingManager.AddCopy(scene->opaqueDrawIndirectCommands, scene->opaqueDrawIndirectCommandsBuffer->GetBuffer());
    stagingManager.AddCopy(scene->opaqueDrawData, scene->opaqueDrawDataBuffer->GetBuffer());

    stagingManager.AddCopy(scene->transparentDrawIndirectCommands,
                           scene->transparentDrawIndirectCommandsBuffer->GetBuffer());
    stagingManager.AddCopy(scene->transparentDrawData, scene->transparentDrawDataBuffer->GetBuffer());

    stagingManager.AddCopy(scene->globalModelMatrices, scene->modelMatricesBuffer->GetBuffer());
}

void Application::FindScenePaths(const std::filesystem::path &basePath) {
    for (const auto &dirEntry: std::filesystem::recursive_directory_iterator(basePath)) {
        if (dirEntry.is_regular_file() &&
            (dirEntry.path().extension() == ".gltf" || dirEntry.path().extension() == ".glb")) {
            scenePaths.push_back(dirEntry.path());
        }
    }
}
