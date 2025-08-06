#include "pch.h"

#include "DebugDraw.h"

#include "Application.h"
#include "GPUDataUploader.h"

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
void DebugDraw::DrawAABB(const AABB &aabb, const glm::vec3 &color) {
    glm::vec3 min = aabb.min;
    glm::vec3 max = aabb.max;

    // Bottom square
    DrawLine({min.x, min.y, min.z}, {max.x, min.y, min.z}, color);
    DrawLine({max.x, min.y, min.z}, {max.x, min.y, max.z}, color);
    DrawLine({max.x, min.y, max.z}, {min.x, min.y, max.z}, color);
    DrawLine({min.x, min.y, max.z}, {min.x, min.y, min.z}, color);

    // Top square
    DrawLine({min.x, max.y, min.z}, {max.x, max.y, min.z}, color);
    DrawLine({max.x, max.y, min.z}, {max.x, max.y, max.z}, color);
    DrawLine({max.x, max.y, max.z}, {min.x, max.y, max.z}, color);
    DrawLine({min.x, max.y, max.z}, {min.x, max.y, min.z}, color);

    // Vertical edges
    DrawLine({min.x, min.y, min.z}, {min.x, max.y, min.z}, color);
    DrawLine({max.x, min.y, min.z}, {max.x, max.y, min.z}, color);
    DrawLine({max.x, min.y, max.z}, {max.x, max.y, max.z}, color);
    DrawLine({min.x, min.y, max.z}, {min.x, max.y, max.z}, color);
}

void DebugDraw::DrawFrustum(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &color) {
    // Clip space corners
    constexpr static std::array clipCorners = {
        glm::vec4(-1, -1, 0, 1), // Near bottom-left
        glm::vec4(1, -1, 0, 1),  // Near bottom-right
        glm::vec4(1, 1, 0, 1),   // Near top-right
        glm::vec4(-1, 1, 0, 1),  // Near top-left
        glm::vec4(-1, -1, 1, 1), // Far bottom-left
        glm::vec4(1, -1, 1, 1),  // Far bottom-right
        glm::vec4(1, 1, 1, 1),   // Far top-right
        glm::vec4(-1, 1, 1, 1),  // Far top-left
    };

    const glm::mat4 invViewProj = glm::inverse(proj * view);
    std::array<glm::vec3, 8> worldCorners;

    for (size_t i = 0; i < 8; ++i) {
        glm::vec4 worldPos = invViewProj * clipCorners[i];
        worldCorners[i] = glm::vec3(worldPos) / worldPos.w;
    }

    // Near plane
    DrawLine(worldCorners[0], worldCorners[1], color);
    DrawLine(worldCorners[1], worldCorners[2], color);
    DrawLine(worldCorners[2], worldCorners[3], color);
    DrawLine(worldCorners[3], worldCorners[0], color);

    // Far plane
    DrawLine(worldCorners[4], worldCorners[5], color);
    DrawLine(worldCorners[5], worldCorners[6], color);
    DrawLine(worldCorners[6], worldCorners[7], color);
    DrawLine(worldCorners[7], worldCorners[4], color);

    // Connect near and far planes
    DrawLine(worldCorners[0], worldCorners[4], color);
    DrawLine(worldCorners[1], worldCorners[5], color);
    DrawLine(worldCorners[2], worldCorners[6], color);
    DrawLine(worldCorners[3], worldCorners[7], color);
}

void DebugDraw::Draw(VkCommandBuffer commandBuffer, GPUDataUploader &stagingManager, const VulkanPipeline &debugPipeline,
                     const Scene &scene, const VkRenderingInfo &renderingInfo) {
    if (vertices.empty()) {
        return;
    }

    stagingManager.AddCopy(vertices, vertexBuffer->GetBuffer());
    stagingManager.Flush(commandBuffer);

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
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
