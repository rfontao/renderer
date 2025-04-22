#version 460

#include "common.glsl"

layout (push_constant, scalar) uniform PushConsts {
    vec3 color;
    CameraBuffer cameraBufferAddress;
    int cameraIndex;
    vec3 padding;
} pc;

layout (location = 0) in vec3 i_Position;

layout (location = 0) out vec3 o_Color;

void main() {
    Camera camera = pc.cameraBufferAddress.cameras[pc.cameraIndex];

    gl_Position = camera.proj * camera.view * vec4(i_Position, 1.0);
    o_Color = pc.color;
}
