#include "pch.h"
#include "camera.h"

Camera::Camera(const glm::vec3 &position, const glm::vec3 &worldUp,
               const glm::vec3 &focusPoint) : m_Position(position), m_WorldUp(worldUp),
                                              m_FocusPoint(focusPoint), m_Up(worldUp) {
    UpdateVectors();
}

void Camera::UpdateVectors() {
    glm::vec3 front = glm::normalize(m_FocusPoint - m_Position);

    m_Right = glm::normalize(glm::cross(front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, front));
}

glm::mat4 Camera::GetViewMatrix() {
    return glm::lookAt(m_Position, m_FocusPoint, m_Up);
}

void Camera::SetMove(bool move) {
    m_MoveCamera = move;
    if (!m_MoveCamera) {
        m_FirstMouse = true;
    }
}

void Camera::HandleMouseMovement(float xPos, float yPos) {
    if (!m_MoveCamera)
        return;

    if (m_FirstMouse) {
        m_LastMouseX = xPos;
        m_LastMouseY = yPos;
        m_FirstMouse = false;
    }

    float xOffset = xPos - m_LastMouseX;
    float yOffset = m_LastMouseY - yPos; // reversed since y-coordinates go from bottom to top

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
