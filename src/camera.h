#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class Camera {
public:
    Camera(const glm::vec3 &position, const glm::vec3 &worldUp, const glm::vec3 &focusPoint, double yaw = 0.0f,
           double pitch = 0.0f);

    Camera() : Camera(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)) {}

public:
    glm::mat4 GetViewMatrix();

    void ProcessMouseMovement(double xoffset, double yoffset);

private:
    void UpdateVectors();

    glm::vec3 position;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    glm::vec3 focusPoint;

    double yaw;
    double pitch;
};
