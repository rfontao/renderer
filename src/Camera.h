#pragma once

struct Frustum {
    std::array<glm::vec4, 6> planes; // left, right, top, bottom, near, far
};

class Camera {
public:
    enum CameraMode { LOOKAT, FREE };
    enum MovementDirection { FRONT, BACK, LEFT, RIGHT };

    Camera() = default;
    Camera(const glm::vec3 &position, const glm::vec3 &worldUp, const glm::vec3 &focusPoint, double aspectRatio,
           double yFov = 45.0f);

    [[nodiscard]] glm::mat4 GetViewMatrix() const;
    [[nodiscard]] glm::mat4 GetProjectionMatrix() const;
    [[nodiscard]] glm::vec3 GetPosition() const { return m_Position; }
    [[nodiscard]] Frustum GetFrustum() const { return m_Frustum; }

    void SetAspectRatio(double aspectRatio) { m_AspectRatio = aspectRatio; }

    void SetMove(bool move);
    void HandleMouseMovement(double xPos, double yPos);
    void HandleMouseScroll(double scrollAmount);
    void HandleMovement(MovementDirection direction);

    struct CameraData {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 position;
    };

    [[nodiscard]] CameraData GetCameraData() const;

    bool DoesSphereIntersectFrustum(glm::vec4 sphere) const;

    std::vector<glm::vec3> GenerateFrustumVertices();
    static std::vector<uint32_t> GenerateFrustumLineIndices();

private:
    void UpdateVectors();
    void UpdateFrustum();

    CameraMode m_Mode = FREE;

    glm::vec4 transform;

    glm::vec3 m_Position;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;
    glm::vec3 m_FocusPoint;

    double m_AspectRatio;
    double m_YFov;

    bool m_FirstMouse = true;
    double m_LastMouseX = 0.0f;
    double m_LastMouseY = 0.0f;
    bool m_MoveCamera = false; // Right mouse button is pressed

    double m_MouseSensitivity = 0.005f;

    Frustum m_Frustum;
};
