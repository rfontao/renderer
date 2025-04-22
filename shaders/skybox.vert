#version 460

#include "common.glsl"

layout (location = 0) in vec3 i_Position;
layout (location = 1) in vec3 i_Normal;
layout (location = 2) in vec3 i_Color;
layout (location = 3) in vec2 i_UV0;
layout (location = 4) in vec2 i_UV1;

layout (location = 0) out vec3 o_TexCoords;

layout (push_constant, scalar) uniform PushConsts {
    CameraBuffer cameraBufferAddress;
    int cameraIndex;
    int skyboxTextureIndex;
    int padding;
} pc;

void main() {
    Camera camera = pc.cameraBufferAddress.cameras[pc.cameraIndex];

    mat4 viewNoTranslation = mat4(mat3(camera.view));
    gl_Position = camera.proj * viewNoTranslation * vec4(i_Position.xyz, 1.0);
    o_TexCoords = i_Position;
}