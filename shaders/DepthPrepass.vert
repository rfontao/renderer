#version 460

#include "common.glsl"

layout (push_constant, scalar) uniform PushConsts {
    CameraBuffer cameraBufferAddress;
    DrawDataBuffer drawDataBufferAddress;
    ModelMatricesBuffer modelMatricesBufferAddress;
    int cameraIndex;
    int padding;
} pc;

layout (location = 0) in vec3 i_Position;
layout (location = 1) in vec3 i_Normal;
layout (location = 2) in vec3 i_Color;
layout (location = 3) in vec2 i_UV0;
layout (location = 4) in vec2 i_UV1;

void main() {
    Camera camera = pc.cameraBufferAddress.cameras[pc.cameraIndex];

    DrawData drawData = pc.drawDataBufferAddress.drawData[gl_DrawID];
    mat4 modelMatrix = pc.modelMatricesBufferAddress.matrices[drawData.modelMatrixIndex];

    gl_Position = camera.proj * camera.view * modelMatrix * vec4(i_Position, 1.0);
}
