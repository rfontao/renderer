#include "Camera.h"
#include "pch.h"

Camera::Camera(const glm::vec3 &position, const glm::vec3 &worldUp, const glm::vec3 &focusPoint, double aspectRatio,
               double yFov) :
    m_Position(position), m_WorldUp(worldUp), m_FocusPoint(focusPoint), m_Up(worldUp), m_AspectRatio(aspectRatio),
    m_YFov(yFov) {
    UpdateVectors();
}

void Camera::UpdateVectors() {
    glm::vec3 front = glm::normalize(m_FocusPoint - m_Position);

    m_Right = glm::normalize(glm::cross(front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, front));

    UpdateFrustum();
}

// https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook-Second-Edition/blob/main/shared/UtilsMath.h
void Camera::UpdateFrustum() {
    auto proj = GetProjectionMatrix();
    proj[1][1] *= -1; // Vulkan uses a different coordinate system for the projection matrix
    const auto viewProj = proj * GetViewMatrix();
    m_Frustum.planes[0] = glm::vec4(viewProj[3] + viewProj[0]); // left
    m_Frustum.planes[1] = glm::vec4(viewProj[3] - viewProj[0]); // right
    m_Frustum.planes[2] = glm::vec4(viewProj[3] + viewProj[1]); // bottom
    m_Frustum.planes[3] = glm::vec4(viewProj[3] - viewProj[1]); // top
    m_Frustum.planes[4] = glm::vec4(viewProj[3] + viewProj[2]); // near
    m_Frustum.planes[5] = glm::vec4(viewProj[3] - viewProj[2]); // far
}

bool Camera::DoesSphereIntersectFrustum(glm::vec4 sphere) const {
    for (const auto &plane: m_Frustum.planes) {
        // Check if the sphere is inside the frustum
        if (plane.x * sphere.x + plane.y * sphere.y + plane.z * sphere.z + plane.w <= -sphere.w) {
            return false;
        }
    }
    return true;
}

std::vector<glm::vec3> Camera::GenerateFrustumVertices() {
    UpdateVectors();

    // Frustum corners in NDC
    static std::vector<glm::vec4> ndcCorners = {
            {-1, -1, -1, 1}, {1, -1, -1, 1}, {1, 1, -1, 1}, {-1, 1, -1, 1}, // Near plane
            {-1, -1, 1, 1},  {1, -1, 1, 1},  {1, 1, 1, 1},  {-1, 1, 1, 1} // Far plane
    };

    auto proj = GetProjectionMatrix();
    proj[1][1] *= -1; // Vulkan uses a different coordinate system for the projection matrix
    const glm::mat4 invVP = glm::inverse(proj * GetViewMatrix());

    std::vector<glm::vec3> frustumVertices;
    for (const auto &corner: ndcCorners) {
        glm::vec4 worldPos = invVP * corner;
        frustumVertices.push_back(glm::vec3(worldPos) / worldPos.w); // Perspective divide
    }

    return frustumVertices;
}

std::vector<uint32_t> Camera::GenerateFrustumLineIndices() {
    return {
            0, 1, 1, 2, 2, 3, 3, 0, // Near plane
            4, 5, 5, 6, 6, 7, 7, 4, // Far plane
            0, 4, 1, 5, 2, 6, 3, 7 // Connecting lines
    };
}

glm::mat4 Camera::GetViewMatrix() const { return glm::lookAt(m_Position, m_FocusPoint, m_Up); }

glm::mat4 Camera::GetProjectionMatrix() const {
    glm::mat4 proj = glm::perspective(glm::radians(m_YFov), m_AspectRatio, 0.5, 1000.0);
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

    if (m_Mode == FREE) {

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

        UpdateVectors();
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
        UpdateVectors();
    }
}

void Camera::HandleMovement(MovementDirection direction) {
    if (!m_MoveCamera)
        return;

    if (m_Mode == LOOKAT)
        return;

    constexpr float cameraMovementSpeed = 0.005f;
    glm::vec3 front = m_Position - m_FocusPoint;
    switch (direction) {
        case FRONT:
            m_Position = m_Position - front * cameraMovementSpeed;
            m_FocusPoint = m_FocusPoint - front * cameraMovementSpeed;
            break;
        case BACK:
            m_Position = m_Position + front * cameraMovementSpeed;
            m_FocusPoint = m_FocusPoint + front * cameraMovementSpeed;
            break;
        case LEFT:
            m_Position = m_Position - m_Right * cameraMovementSpeed;
            m_FocusPoint = m_FocusPoint - m_Right * cameraMovementSpeed;
            break;
        case RIGHT:
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
