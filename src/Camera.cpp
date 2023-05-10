#include "pch.h"
#include "Camera.h"

Camera::Camera(const glm::vec3 &position, const glm::vec3 &worldUp,
               const glm::vec3 &focusPoint, double aspectRatio,
               double yFov) : m_Position(position), m_WorldUp(worldUp),
                              m_FocusPoint(focusPoint), m_Up(worldUp),
                              m_AspectRatio(aspectRatio), m_YFov(yFov) {
    UpdateVectors();
}

void Camera::UpdateVectors() {
    glm::vec3 front = glm::normalize(m_FocusPoint - m_Position);

    m_Right = glm::normalize(glm::cross(front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, front));
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_Position, m_FocusPoint, m_Up);
}

glm::mat4 Camera::GetProjectionMatrix() const {
    glm::mat4 proj = glm::perspective(glm::radians(m_YFov), m_AspectRatio, 0.1, 500.0);
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

    const float cameraMovementSpeed = 0.005f;
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
}

void Camera::HandleMouseScroll(double scrollAmount) {
    const double zoomSensitivity = 1.0;
    m_YFov -= scrollAmount * zoomSensitivity;
}
