#include "pch.h"
#include "UI.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

UI::UI(std::shared_ptr<VulkanDevice> device, VkInstance instance, GLFWwindow *window) : m_Device(device) {

    VkDescriptorPoolSize pool_sizes[] =
            {
                    {VK_DESCRIPTOR_TYPE_SAMPLER,                1000},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000}
            };
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    poolInfo.poolSizeCount = (uint32_t) IM_ARRAYSIZE(pool_sizes);
    poolInfo.pPoolSizes = pool_sizes;
    VK_CHECK(vkCreateDescriptorPool(m_Device->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool),
             "Failed to allocate UI descriptor pool");


    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = m_Device->GetPhysicalDevice();
    initInfo.Device = m_Device->GetDevice();
    initInfo.Queue = m_Device->GetGraphicsQueue();
    initInfo.DescriptorPool = m_DescriptorPool;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = 2;
    initInfo.UseDynamicRendering = true;
    initInfo.ColorAttachmentFormat = VK_FORMAT_B8G8R8A8_SRGB;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&initInfo, VK_NULL_HANDLE);


    VkCommandBuffer cmd = m_Device->BeginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(cmd);
    m_Device->EndSingleTimeCommands(cmd);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void UI::Destroy() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(m_Device->GetDevice(), m_DescriptorPool, nullptr);
}

void UI::Draw(VkCommandBuffer cmd) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::EndFrame();
    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}
