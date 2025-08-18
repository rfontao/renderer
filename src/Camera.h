#pragma once

struct Frustum {
    std::array<glm::vec4, 6> planes; // left, right, top, bottom, near, far
};

struct AABB;

class Camera {
public:
    enum struct CameraMode { LOOKAT, FREE };
    enum struct MovementDirection { FRONT, BACK, LEFT, RIGHT };

    Camera() = default;
    Camera(const glm::vec3 &position, const glm::vec3 &worldUp, const glm::vec3 &focusPoint, double aspectRatio,
           double yFov = 45.0f);

    [[nodiscard]] glm::mat4 GetViewMatrix() const;
    [[nodiscard]] glm::mat4 GetProjectionMatrix() const;
    [[nodiscard]] glm::vec3 GetPosition() const { return position; }
    [[nodiscard]] Frustum GetFrustum() const { return frustum; }

    void SetAspectRatio(double aspectRatio) { this->aspectRatio = aspectRatio; }

    void SetMove(bool move);
    void HandleMouseMovement(double xPos, double yPos);
    void HandleMouseScroll(double scrollAmount);
    void HandleMovement(MovementDirection direction);

    struct GPUData {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 position;
    };

    [[nodiscard]] GPUData GetGPUData() const;

    [[nodiscard]] bool IsAABBFullyOutsideFrustum(const AABB &aabb) const;

    glm::vec3 focusPoint;

private:
    void UpdateVectors();
    void UpdateFrustum();

    CameraMode mode = CameraMode::FREE;

    glm::vec4 transform;

    glm::vec3 position;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    double aspectRatio;
    double yFov;

    bool firstMouse = true;
    double lastMouseX = 0.0f;
    double lastMouseY = 0.0f;
    bool moveCamera = false; // Right mouse button is pressed

    double mouseSensitivity = 0.005f;

    Frustum frustum;
};
