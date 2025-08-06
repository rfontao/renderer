#include "pch.h"

#include <span>
#include "UI.h"

#include "Vulkan/Utils.h"

#include "Application.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

UI::UI(std::shared_ptr<VulkanDevice> device, VkInstance instance, GLFWwindow *window, Application *app) :
    device(std::move(device)), app(app) {
    constexpr VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = (uint32_t) IM_ARRAYSIZE(pool_sizes);
    poolInfo.pPoolSizes = pool_sizes;
    VK_CHECK(vkCreateDescriptorPool(this->device->GetDevice(), &poolInfo, nullptr, &descriptorPool),
             "Failed to allocate UI descriptor pool");


    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Washed out colors workaround due to sRGB issues
    // (https://github.com/ocornut/imgui/issues/1927#issuecomment-937519584)
    const auto ToLinear = [](float x) {
        if (x <= 0.04045f) {
            return x / 12.92f;
        } else {
            return std::pow((x + 0.055f) / 1.055f, 2.4f);
        }
    };
    for (auto &c: std::span{ImGui::GetStyle().Colors, size_t(ImGuiCol_COUNT)}) {
        c = {ToLinear(c.x), ToLinear(c.y), ToLinear(c.z), c.w};
    }

    std::array<VkFormat, 1> imageFormat{app->swapchain->GetImageFormat()};
    VkPipelineRenderingCreateInfo pipelineRenderingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = imageFormat.data(),
            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
    };

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = this->device->GetPhysicalDevice();
    initInfo.Device = this->device->GetDevice();
    initInfo.Queue = this->device->GetGraphicsQueue();
    initInfo.DescriptorPool = descriptorPool;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = 2;
    initInfo.UseDynamicRendering = true;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.PipelineRenderingCreateInfo = pipelineRenderingInfo;
    ImGui_ImplVulkan_Init(&initInfo);

    VkCommandBuffer cmd = this->device->BeginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture();
    this->device->EndSingleTimeCommands(cmd);

    ImGui_ImplVulkan_DestroyFontsTexture();
}

void UI::Destroy() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(device->GetDevice(), descriptorPool, nullptr);
}

void UI::Draw(VkCommandBuffer cmd) {
    static std::string selectedSceneName;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("General information");

    ImGuiIO &io = ImGui::GetIO();

    ImGui::Text("Frame time: %.3f ms", 1000.0f / io.Framerate);
    ImGui::Text("FPS: %.1f", io.Framerate);

    ImGui::Separator();

    if (ImGui::BeginCombo("Scene", selectedSceneName.c_str(), 0)) {
        auto paths = app->GetScenePaths();
        for (const auto &path: paths) {
            auto pathString = path.filename().string();
            auto filename = pathString.c_str();
            bool is_selected = selectedSceneName == filename;
            if (ImGui::Selectable(filename, is_selected)) {
                selectedSceneName = filename;
                app->SetScene(path);
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::End();

    ImGui::EndFrame();
    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}
