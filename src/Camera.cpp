#include "pch.h"

#include "Camera.h"
#include "Scene.h"

Camera::Camera(const glm::vec3 &position, const glm::vec3 &worldUp, const glm::vec3 &focusPoint,
               const double aspectRatio, const double yFov) :
    focusPoint(focusPoint), position(position), up(worldUp), worldUp(worldUp), aspectRatio(aspectRatio),
    yFov(yFov) {
    UpdateVectors();
}

void Camera::UpdateVectors() {
    const glm::vec3 front = glm::normalize(focusPoint - position);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));

    UpdateFrustum();
}

// https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook-Second-Edition/blob/main/shared/UtilsMath.h
void Camera::UpdateFrustum() {
    const auto viewProj = glm::transpose(GetProjectionMatrix() * GetViewMatrix());
    frustum.planes[0] = glm::vec4(viewProj[3] + viewProj[0]); // left
    frustum.planes[1] = glm::vec4(viewProj[3] - viewProj[0]); // right
    frustum.planes[2] = glm::vec4(viewProj[3] + viewProj[1]); // bottom
    frustum.planes[3] = glm::vec4(viewProj[3] - viewProj[1]); // top
    frustum.planes[4] = glm::vec4(viewProj[3] + viewProj[2]); // near
    frustum.planes[5] = glm::vec4(viewProj[3] - viewProj[2]); // far

    // Normalize planes
    for (auto &plane: frustum.planes) {
        const float length = glm::length(glm::vec3(plane));
        plane /= length;
    }
}

bool Camera::IsAABBFullyOutsideFrustum(const AABB &aabb) const {
    // Realtime rendering 3rd edition book section 22.10.1
    const glm::vec3 center = (aabb.min + aabb.max) * 0.5f;
    const glm::vec3 halfSize = (aabb.max - aabb.min) * 0.5f;

    for (const auto &plane: frustum.planes) {
        const float extent =
                halfSize.x * std::abs(plane.x) + halfSize.y * std::abs(plane.y) + halfSize.z * std::abs(plane.z);

        const float s = glm::dot(glm::vec3(plane), center) + plane.w;
        if (s < -extent) {
            // The AABB is fully outside the frustum
            return true;
        }
    }
    return false;
}

glm::mat4 Camera::GetViewMatrix() const { return glm::lookAt(position, focusPoint, up); }

glm::mat4 Camera::GetProjectionMatrix() const {
    glm::mat4 proj = glm::perspective(glm::radians(yFov), aspectRatio, 0.5, 150.0);
    proj[1][1] *= -1;
    return proj;
}

void Camera::SetMove(bool move) {
    moveCamera = move;
    if (!moveCamera) {
        firstMouse = true;
    }
}

void Camera::HandleMouseMovement(double xPos, double yPos) {
    if (!moveCamera)
        return;

    if (firstMouse) {
        lastMouseX = xPos;
        lastMouseY = yPos;
        firstMouse = false;
    }

    if (mode == CameraMode::FREE) {

        double xOffset = xPos - lastMouseX;
        double yOffset = lastMouseY - yPos; // reversed since y-coordinates go from bottom to top

        lastMouseX = xPos;
        lastMouseY = yPos;

        double yaw = mouseSensitivity * xOffset;
        double pitch = mouseSensitivity * yOffset;

        auto rot = glm::rotate(glm::mat4(1.0f), (float) yaw, up);
        rot = glm::rotate(rot, (float) -pitch, right);


        glm::vec3 front = position - focusPoint;
        front = glm::vec3(glm::vec4(front, 1.0f) * rot);
        focusPoint = position - front;
    } else {
        double xOffset = xPos - lastMouseX;
        double yOffset = lastMouseY - yPos; // reversed since y-coordinates go from bottom to top

        lastMouseX = xPos;
        lastMouseY = yPos;

        double yaw = mouseSensitivity * xOffset;
        double pitch = mouseSensitivity * yOffset;

        auto rot = glm::rotate(glm::mat4(1.0f), (float) yaw, up);
        rot = glm::rotate(rot, (float) pitch, right);

        glm::vec3 front = position - focusPoint;
        front = glm::vec3(glm::vec4(front, 1.0f) * rot);
        position = focusPoint + front;
    }
    UpdateVectors();
}

void Camera::HandleMovement(MovementDirection direction) {
    if (!moveCamera)
        return;

    if (mode == CameraMode::LOOKAT)
        return;

    constexpr float cameraMovementSpeed = 0.005f;
    glm::vec3 front = position - focusPoint;
    switch (direction) {
        case MovementDirection::FRONT:
            position = position - front * cameraMovementSpeed;
            focusPoint = focusPoint - front * cameraMovementSpeed;
            break;
        case MovementDirection::BACK:
            position = position + front * cameraMovementSpeed;
            focusPoint = focusPoint + front * cameraMovementSpeed;
            break;
        case MovementDirection::LEFT:
            position = position - right * cameraMovementSpeed;
            focusPoint = focusPoint - right * cameraMovementSpeed;
            break;
        case MovementDirection::RIGHT:
            position = position + right * cameraMovementSpeed;
            focusPoint = focusPoint + right * cameraMovementSpeed;
            break;
    }

    UpdateVectors();
}

void Camera::HandleMouseScroll(double scrollAmount) {
    constexpr double zoomSensitivity = 1.0;
    yFov -= scrollAmount * zoomSensitivity;
    UpdateVectors();
}

Camera::GPUData Camera::GetGPUData() const {
    return {
            .view = GetViewMatrix(),
            .proj = GetProjectionMatrix(),
            .position = position,
    };
}
