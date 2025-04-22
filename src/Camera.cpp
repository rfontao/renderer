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

void Camera::UpdateFrustum() {
    const auto matrix = GetProjectionMatrix() * GetViewMatrix();

    // https://github.com/SaschaWillems/Vulkan/blob/master/base/frustum.hpp
    // NOTE: the normal vectors are pointing outwards

    // Left plane
    m_Frustum.planes[0] = {matrix[0].w + matrix[0].x, matrix[1].w + matrix[1].x, matrix[2].w + matrix[2].x,
                           matrix[3].w + matrix[3].x};

    // Right plane
    m_Frustum.planes[1] = {matrix[0].w - matrix[0].x, matrix[1].w - matrix[1].x, matrix[2].w - matrix[2].x,
                           matrix[3].w - matrix[3].x};

    // Top plane
    m_Frustum.planes[2] = {matrix[0].w - matrix[0].y, matrix[1].w - matrix[1].y, matrix[2].w - matrix[2].y,
                           matrix[3].w - matrix[3].y};

    // Bottom plane
    m_Frustum.planes[3] = {matrix[0].w + matrix[0].y, matrix[1].w + matrix[1].y, matrix[2].w + matrix[2].y,
                           matrix[3].w + matrix[3].y};

    // Near plane
    m_Frustum.planes[4] = {matrix[0].w + matrix[0].z, matrix[1].w + matrix[1].z, matrix[2].w + matrix[2].z,
                           matrix[3].w + matrix[3].z};

    // Far plane
    m_Frustum.planes[5] = {matrix[0].w - matrix[0].z, matrix[1].w - matrix[1].z, matrix[2].w - matrix[2].z,
                           matrix[3].w - matrix[3].z};

    // Normalize the planes
    for (auto &plane: m_Frustum.planes) {
        const float length = std::sqrt(plane.a * plane.a + plane.b * plane.b + plane.c * plane.c);
        plane.a /= length;
        plane.b /= length;
        plane.c /= length;
        plane.d /= length;
    }
}

bool Camera::DoesSphereIntersectFrustum(glm::vec4 sphere) const {
    for (const auto &plane: m_Frustum.planes) {
        // Check if the sphere is inside the frustum
        if (plane.a * sphere.x + plane.b * sphere.y + plane.c * sphere.z + plane.d <= -sphere.w) {
            return false;
        }
    }
    return true;
}

glm::mat4 Camera::GetViewMatrix() const { return glm::lookAt(m_Position, m_FocusPoint, m_Up); }

glm::mat4 Camera::GetProjectionMatrix() const {
    glm::mat4 proj = glm::perspective(glm::radians(m_YFov), m_AspectRatio, 0.1, 10000.0);
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

std::vector<glm::vec3> Camera::GenerateFrustumVertices() {
    UpdateVectors();

    // Frustum corners in NDC
    static std::vector<glm::vec4> ndcCorners = {
            {-1, -1, 0, 1}, {1, -1, 0, 1}, {1, 1, 0, 1}, {-1, 1, 0, 1}, // Near plane
            {-1, -1, 1, 1}, {1, -1, 1, 1}, {1, 1, 1, 1}, {-1, 1, 1, 1} // Far plane
    };

    const glm::mat4 invVP = glm::inverse(GetProjectionMatrix() * GetViewMatrix());

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
