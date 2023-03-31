#include "camera.h"

Camera::Camera(const glm::vec3 &position, const glm::vec3 &worldUp,
               const glm::vec3 &focusPoint,
               double yaw, double pitch) : position(position), worldUp(worldUp),
                                           focusPoint(focusPoint), yaw(yaw),
                                           pitch(pitch) {

}

void Camera::UpdateVectors() {
    glm::vec3 front = glm::normalize(focusPoint - position);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

glm::mat4 Camera::GetViewMatrix() {
    return glm::lookAt(position, focusPoint, up);
}

void Camera::ProcessMouseMovement(double xoffset, double yoffset) {
    xoffset *= 0.5;
    yoffset *= 0.5;

    yaw = 0.005 * xoffset;
    pitch = 0.005 * yoffset;

    auto rot = glm::rotate(glm::mat4(1.0f), (float) yaw, up);
    rot = glm::rotate(rot, (float) pitch, right);

    glm::vec3 front = position - focusPoint;
    front = glm::vec3(glm::vec4(front, 1.0f) * rot);
    position = focusPoint + front;

    UpdateVectors();
}
