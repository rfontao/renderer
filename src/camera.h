#pragma once

class Camera {
public:
    Camera(const glm::vec3 &position, const glm::vec3 &worldUp, const glm::vec3 &focusPoint);

    Camera() : Camera(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)) {}

public:
    glm::mat4 GetViewMatrix();

    void SetMove(bool move);

    void HandleMouseMovement(float xPos, float yPos);

private:
    void UpdateVectors();

    glm::vec3 m_Position;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;
    glm::vec3 m_FocusPoint;

    bool  m_FirstMouse = true;
    float m_LastMouseX = 0.0f;
    float m_LastMouseY = 0.0f;
    bool m_MoveCamera = false; // Right mouse button is pressed

    float m_MouseSensitivity = 0.005f;
};
