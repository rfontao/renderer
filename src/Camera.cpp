#include "pch.h"

#include "Camera.h"
#include "Scene.h"

Camera::Camera(const glm::vec3 &position, const glm::vec3 &worldUp, const glm::vec3 &focusPoint,
               const double aspectRatio, const double yFov) :
    m_FocusPoint(focusPoint), m_Position(position), m_Up(worldUp), m_WorldUp(worldUp), m_AspectRatio(aspectRatio),
    m_YFov(yFov) {
    UpdateVectors();
}

void Camera::UpdateVectors() {
    const glm::vec3 front = glm::normalize(m_FocusPoint - m_Position);

    m_Right = glm::normalize(glm::cross(front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, front));

    UpdateFrustum();
}

// https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook-Second-Edition/blob/main/shared/UtilsMath.h
void Camera::UpdateFrustum() {
    const auto viewProj = glm::transpose(GetProjectionMatrix() * GetViewMatrix());
    m_Frustum.planes[0] = glm::vec4(viewProj[3] + viewProj[0]); // left
    m_Frustum.planes[1] = glm::vec4(viewProj[3] - viewProj[0]); // right
    m_Frustum.planes[2] = glm::vec4(viewProj[3] + viewProj[1]); // bottom
    m_Frustum.planes[3] = glm::vec4(viewProj[3] - viewProj[1]); // top
    m_Frustum.planes[4] = glm::vec4(viewProj[3] + viewProj[2]); // near
    m_Frustum.planes[5] = glm::vec4(viewProj[3] - viewProj[2]); // far

    // Normalize planes
    for (auto &plane: m_Frustum.planes) {
        const float length = glm::length(glm::vec3(plane));
        plane /= length;
    }
}

bool Camera::IsAABBFullyOutsideFrustum(const AABB &aabb) const {
    // Realtime rendering 3rd edition book section 22.10.1
    const glm::vec3 center = (aabb.min + aabb.max) * 0.5f;
    const glm::vec3 halfSize = (aabb.max - aabb.min) * 0.5f;

    for (const auto &plane: m_Frustum.planes) {
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

glm::mat4 Camera::GetViewMatrix() const { return glm::lookAt(m_Position, m_FocusPoint, m_Up); }

glm::mat4 Camera::GetProjectionMatrix() const {
    glm::mat4 proj = glm::perspective(glm::radians(m_YFov), m_AspectRatio, 0.5, 150.0);
    proj[1][1] *= -1;
    return proj;
}

void Camera::SetMove(bool move) {
    m_MoveCamera = move;
    if (!m_MoveCamera) {
        m_FirstMouse = true;
    }
}

void Camera::HandleMouseMovement(double xPos, double yPos) {
    if (!m_MoveCamera)
        return;

    if (m_FirstMouse) {
        m_LastMouseX = xPos;
        m_LastMouseY = yPos;
        m_FirstMouse = false;
    }

    if (m_Mode == CameraMode::FREE) {

        double xOffset = xPos - m_LastMouseX;
        double yOffset = m_LastMouseY - yPos; // reversed since y-coordinates go from bottom to top

        m_LastMouseX = xPos;
        m_LastMouseY = yPos;

        double yaw = m_MouseSensitivity * xOffset;
        double pitch = m_MouseSensitivity * yOffset;

        auto rot = glm::rotate(glm::mat4(1.0f), (float) yaw, m_Up);
        rot = glm::rotate(rot, (float) -pitch, m_Right);


        glm::vec3 front = m_Position - m_FocusPoint;
        front = glm::vec3(glm::vec4(front, 1.0f) * rot);
        m_FocusPoint = m_Position - front;
    } else {
        double xOffset = xPos - m_LastMouseX;
        double yOffset = m_LastMouseY - yPos; // reversed since y-coordinates go from bottom to top

        m_LastMouseX = xPos;
        m_LastMouseY = yPos;

        double yaw = m_MouseSensitivity * xOffset;
        double pitch = m_MouseSensitivity * yOffset;

        auto rot = glm::rotate(glm::mat4(1.0f), (float) yaw, m_Up);
        rot = glm::rotate(rot, (float) pitch, m_Right);

        glm::vec3 front = m_Position - m_FocusPoint;
        front = glm::vec3(glm::vec4(front, 1.0f) * rot);
        m_Position = m_FocusPoint + front;
    }
    UpdateVectors();
}

void Camera::HandleMovement(MovementDirection direction) {
    if (!m_MoveCamera)
        return;

    if (m_Mode == CameraMode::LOOKAT)
        return;

    constexpr float cameraMovementSpeed = 0.005f;
    glm::vec3 front = m_Position - m_FocusPoint;
    switch (direction) {
        case MovementDirection::FRONT:
            m_Position = m_Position - front * cameraMovementSpeed;
            m_FocusPoint = m_FocusPoint - front * cameraMovementSpeed;
            break;
        case MovementDirection::BACK:
            m_Position = m_Position + front * cameraMovementSpeed;
            m_FocusPoint = m_FocusPoint + front * cameraMovementSpeed;
            break;
        case MovementDirection::LEFT:
            m_Position = m_Position - m_Right * cameraMovementSpeed;
            m_FocusPoint = m_FocusPoint - m_Right * cameraMovementSpeed;
            break;
        case MovementDirection::RIGHT:
            m_Position = m_Position + m_Right * cameraMovementSpeed;
            m_FocusPoint = m_FocusPoint + m_Right * cameraMovementSpeed;
            break;
    }

    UpdateVectors();
}

void Camera::HandleMouseScroll(double scrollAmount) {
    constexpr double zoomSensitivity = 1.0;
    m_YFov -= scrollAmount * zoomSensitivity;
    UpdateVectors();
}

Camera::CameraData Camera::GetCameraData() const {
    return {
            .view = GetViewMatrix(),
            .proj = GetProjectionMatrix(),
            .position = m_Position,
    };
}
