#include "pch.h"

#include "DebugDraw.h"

#include "Application.h"
#include "StagingManager.h"

void DebugDraw::DrawLine(const glm::vec3 &start, const glm::vec3 &end, const glm::vec3 &color) {
    vertices.push_back({.position = start, .color = color});
    vertices.push_back({.position = end, .color = color});
}

void DebugDraw::DrawAxis(const glm::vec3 &origin, const double length) {
    DrawLine(origin, origin + glm::vec3(length, 0.0, 0.0), {1.0, 0.0, 0.0}); // X axis
    DrawLine(origin, origin + glm::vec3(0.0, length, 0.0), {0.0, 1.0, 0.0}); // Y axis
    DrawLine(origin, origin + glm::vec3(0.0, 0.0, length), {0.0, 0.0, 1.0}); // Z axis
}

void DebugDraw::DrawPoint(const glm::vec3 &coords, const glm::vec3 &color) {
    constexpr double length = 0.02;
    DrawLine(coords + glm::vec3(-length, 0.0, 0.0), coords + glm::vec3(length, 0.0, 0.0), color);
    DrawLine(coords + glm::vec3(0.0, -length, 0.0), coords + glm::vec3(0.0, length, 0.0), color);
    DrawLine(coords + glm::vec3(0.0, 0.0, -length), coords + glm::vec3(0.0, 0.0f, length), color);
}

void DebugDraw::DrawSphere(const glm::vec3 &center, const double radius, const glm::vec3 &color) {
    constexpr int segments = 16;
    // XY plane
    for (int i = 0; i < segments; ++i) {
        double theta1 = static_cast<double>(i) / segments * glm::pi<double>() * 2.0;
        double theta2 = static_cast<double>(i + 1) / segments * glm::pi<double>() * 2.0;
        glm::vec3 p1 = center + glm::vec3(radius * cos(theta1), radius * sin(theta1), 0.0);
        glm::vec3 p2 = center + glm::vec3(radius * cos(theta2), radius * sin(theta2), 0.0);
        DrawLine(p1, p2, color);
    }
    // XZ plane
    for (int i = 0; i < segments; ++i) {
        double theta1 = static_cast<double>(i) / segments * glm::pi<double>() * 2.0;
        double theta2 = static_cast<double>(i + 1) / segments * glm::pi<double>() * 2.0;
        glm::vec3 p1 = center + glm::vec3(radius * cos(theta1), 0.0, radius * sin(theta1));
        glm::vec3 p2 = center + glm::vec3(radius * cos(theta2), 0.0, radius * sin(theta2));
        DrawLine(p1, p2, color);
    }
    // YZ plane
    for (int i = 0; i < segments; ++i) {
        double theta1 = static_cast<double>(i) / segments * glm::pi<double>() * 2.0;
        double theta2 = static_cast<double>(i + 1) / segments * glm::pi<double>() * 2.0;
        glm::vec3 p1 = center + glm::vec3(0.0, radius * cos(theta1), radius * sin(theta1));
        glm::vec3 p2 = center + glm::vec3(0.0, radius * cos(theta2), radius * sin(theta2));
        DrawLine(p1, p2, color);
    }
}

void DebugDraw::Draw(VkCommandBuffer commandBuffer, StagingManager &stagingManager, const VulkanPipeline &debugPipeline,
                     const Scene &scene, const VkRenderingInfo &renderingInfo) {
    if (vertices.empty()) {
        return;
    }

    const VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
    stagingManager.AddCopy(vertices.data(), vertexBuffer->GetBuffer(), bufferSize);
    stagingManager.Flush(commandBuffer);

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) 1280,
            .height = (float) 800,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{
            .offset = {0, 0},
            .extent = {1280, 800},
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipeline.GetPipeline());

    constexpr VkDeviceSize offsets[] = {0};
    const VkBuffer vertexBuffersArray[] = {vertexBuffer->GetBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffersArray, offsets);

    const struct DebugDrawPushConstants {
        VkDeviceAddress cameraBufferAddress;
        int32_t cameraIndex;
    } pushConstants = {scene.camerasBuffer->GetAddress(), 0};
    vkCmdPushConstants(commandBuffer, debugPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants),
                       &pushConstants);
    vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);
    vkCmdEndRendering(commandBuffer);
}

void DebugDraw::EndFrame() {
    vertices.clear();
    currentFrame = (currentFrame + 1) % 2;
}
