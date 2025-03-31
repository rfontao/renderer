#version 460

#include "common.glsl"

layout (scalar, push_constant) uniform PushConsts {
    mat4 model;
    LightsBuffer lightBufferAddress;
    int directionLightIndex;
    int padding; // TODO: not sure why this is needed
} pc;

layout (location = 0) in vec3 i_Position;
layout (location = 1) in vec3 i_Normal;
layout (location = 2) in vec3 i_Color;
layout (location = 3) in vec2 i_UV0;
layout (location = 4) in vec2 i_UV1;

void main() {
    Light directionallight = pc.lightBufferAddress.lights[pc.directionLightIndex];
    gl_Position = directionallight.proj * directionallight.view * pc.model * vec4(i_Position, 1.0);
}
