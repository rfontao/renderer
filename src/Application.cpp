#include "pch.h"

#include "Application.h"
#include "spirv_reflect.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator,
                                             VkDebugUtilsMessengerEXT *pDebugMessenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                          VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks *pAllocator
) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                            "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
    createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback,
    };
}

void Application::Run() {
    m_Camera = Camera(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
//    m_Camera = Camera(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));

//    PrintAvailableVulkanExtensions();
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();
}

void Application::InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();

    VkSurfaceKHR surface = CreateSurface();
    m_Device = std::make_shared<VulkanDevice>(m_Instance, surface);
    m_Swapchain = std::make_shared<VulkanSwapchain>(m_Device, m_Window);
    m_GraphicsPipeline = std::make_shared<VulkanPipeline>(m_Device, m_Swapchain->GetImageFormat());

//    m_Scene = Scene(m_Device, "models/OrientationTest/OrientationTest.gltf");
//    m_Scene = Scene(m_Device, "models/VertexColorTest/VertexColorTest.gltf");
//    m_Scene = Scene(m_Device, "models/BoxVertexColors/BoxVertexColors.gltf");
//    m_Scene = Scene(m_Device, "models/2CylinderEngine/2CylinderEngine.gltf");
//    m_Scene = Scene(m_Device, "models/Sponza/Sponza.gltf");
//    m_Scene = Scene(m_Device, "models/BoxTextured/BoxTextured.gltf");
//    m_Scene = Scene(m_Device, "models/minimal.gltf");
    m_Scene = Scene(m_Device, "models/Box/Box.gltf");


    CreateColorResources();
    CreateDepthResources();

    CreateUniformBuffers();
    CreateDescriptorSets();
}

void Application::MainLoop() {
    while (!glfwWindowShouldClose(m_Window)) {
        glfwPollEvents();
        DrawFrame();
    }

    vkDeviceWaitIdle(m_Device->GetDevice());
}

void Application::Cleanup() {
    m_Swapchain->Destroy();

    colorImage->Destroy();
    depthImage->Destroy();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i]->Destroy();
    }

    m_GraphicsPipeline->Destroy();
    m_Scene.Destroy();

    vkDestroySurfaceKHR(m_Instance, m_Device->GetSurface(), nullptr);
    m_Device->Destroy();

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(m_Instance, debugMessenger, nullptr);
    }

    vkDestroyInstance(m_Instance, nullptr);

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Application::InitWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_Window = glfwCreateWindow((int) WIDTH, (int) HEIGHT, "Experimental Renderer", nullptr, nullptr);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow *w, int width, int height) {
        auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(w));
        app->m_Swapchain->m_NeedsResizing = true;
    });

    glfwSetMouseButtonCallback(m_Window, [](GLFWwindow *w, int button, int action, int mods) {
        auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(w));

        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                app->GetCamera().SetMove(true);
                glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            } else if (action == GLFW_RELEASE) {
                app->GetCamera().SetMove(false);
                glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    });

    glfwSetCursorPosCallback(m_Window,
                             [](GLFWwindow *w, double xPosIn, double yPosIn) {
                                 auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(w));
                                 app->GetCamera().HandleMouseMovement((float) xPosIn, (float) yPosIn);
                             }
    );
}

void Application::CreateInstance() {
    if (enableValidationLayers && !CheckValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Experimental Renderer",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "None",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3
    };

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = (uint32_t) extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = (uint32_t) validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }

}

bool Application::CheckValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

//    std::cout << "Available validation extensions:" << std::endl;
//    for (const auto &layer: availableLayers) {
//        std::cout << '\t' << layer.layerName << std::endl;
//    }

    for (const char *layerName: validationLayers) {
        bool layerFound = false;
        for (const auto &layerProperties: availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
            return false;
    }

    return true;
}

std::vector<const char *> Application::GetRequiredExtensions() const {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void Application::SetupDebugMessenger() {
    if (!enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    PopulateDebugMessengerCreateInfo(createInfo);

    VK_CHECK(CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &debugMessenger),
             "Failed to set up debug messenger!");
}

VkSurfaceKHR Application::CreateSurface() const {
    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &surface), "Failed to create window surface!");
    return surface;
}

void Application::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

    bool msaaEnabled = msaaSamples != VK_SAMPLE_COUNT_1_BIT;
    VkRenderingAttachmentInfo colorAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = msaaEnabled ? colorImage->GetImageView() : m_Swapchain->GetImageView(imageIndex),
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .resolveMode = msaaEnabled ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE,
            .resolveImageView = m_Swapchain->GetImageView(imageIndex),
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
            .renderArea = {0, 0, m_Swapchain->GetWidth(), m_Swapchain->GetHeight()},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
            .pDepthAttachment = &depthAttachment,
//            .pStencilAttachment = &depthAttachment,
    };

    VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    m_Swapchain->GetImage(imageIndex)->TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED,
                                                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    depthImage->TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    vkCmdBeginRendering(commandBuffer, &renderInfo);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->GetPipeline());

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

    // Bind camera matrices
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->GetLayout(), 0, 1,
                            &descriptorSets[currentFrame], 0, nullptr);
    m_Scene.Draw(commandBuffer, m_GraphicsPipeline->GetLayout());

    vkCmdEndRendering(commandBuffer);

    m_Swapchain->GetImage(imageIndex)->TransitionLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer!");
}

void Application::DrawFrame() {
    vkWaitForFences(m_Device->GetDevice(), 1, &m_Swapchain->GetWaitFences()[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = m_Swapchain->AcquireNextImage(currentFrame);
    // Recreate swapchain
    if (imageIndex == std::numeric_limits<uint32_t>::max()) {
        colorImage->Destroy();
        CreateColorResources();
        depthImage->Destroy();
        CreateDepthResources();
        return;
    }

    UpdateUniformBuffer(currentFrame);
    vkResetFences(m_Device->GetDevice(), 1, &m_Swapchain->GetWaitFences()[currentFrame]);

    vkResetCommandBuffer(m_Swapchain->GetCommandBuffers()[currentFrame], 0);
    RecordCommandBuffer(m_Swapchain->GetCommandBuffers()[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    };

    VkSemaphore waitSemaphores[] = {m_Swapchain->GetImageAvailableSemaphores()[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_Swapchain->GetCommandBuffers()[currentFrame];

    VkSemaphore signalSemaphores[] = {m_Swapchain->GetRenderFinishedSemaphores()[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_CHECK(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_Swapchain->GetWaitFences()[currentFrame]),
             "Failed to submit draw command buffer!");

    bool resourceNeedResizing = m_Swapchain->Present(imageIndex, currentFrame);
    if (resourceNeedResizing) {
        colorImage->Destroy();
        CreateColorResources();
        depthImage->Destroy();
        CreateDepthResources();
    }
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::CreateUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i] = std::make_shared<VulkanBuffer>(m_Device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        uniformBuffers[i]->Map();
    }

}

void Application::UpdateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

//    auto currentTime = std::chrono::high_resolution_clock::now();
//    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.view = m_Camera.GetViewMatrix();
    ubo.proj = glm::perspective(glm::radians(45.0f), (float) m_Swapchain->GetWidth() / (float) m_Swapchain->GetHeight(),
                                0.1f,
                                500.0f);

    ubo.proj[1][1] *= -1;

    uniformBuffers[currentImage]->From(&ubo, sizeof(ubo));
}


void Application::CreateDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_GraphicsPipeline->GetDescriptorSetLayouts()[0]);
    VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_Device->GetDescriptorPool(),
            .descriptorSetCount = (uint32_t) MAX_FRAMES_IN_FLIGHT,
            .pSetLayouts = layouts.data(),
    };

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    VK_CHECK(vkAllocateDescriptorSets(m_Device->GetDevice(), &allocInfo, descriptorSets.data()),
             "Failed to allocate descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{
                .buffer = uniformBuffers[i]->GetBuffer(),
                .offset = 0,
                .range = sizeof(UniformBufferObject),
        };

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_Device->GetDevice(), (uint32_t) descriptorWrites.size(), descriptorWrites.data(), 0,
                               nullptr);
    }
}

void Application::CreateDepthResources() {
    VkFormat depthFormat = m_Device->FindDepthFormat();

    depthImage = std::make_shared<VulkanImage>(m_Device, m_Swapchain->GetWidth(), m_Swapchain->GetHeight(), 1,
                                               msaaSamples,
                                               depthFormat,
                                               VK_IMAGE_TILING_OPTIMAL,
                                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                               VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Application::CreateColorResources() {
    VkFormat colorFormat = m_Swapchain->GetImageFormat();

    colorImage = std::make_shared<VulkanImage>(m_Device, m_Swapchain->GetWidth(), m_Swapchain->GetHeight(), 1,
                                               msaaSamples,
                                               colorFormat,
                                               VK_IMAGE_TILING_OPTIMAL,
                                               VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
}

Camera &Application::GetCamera() {
    return m_Camera;
}


