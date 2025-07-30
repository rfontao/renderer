#version 460

#include "common.glsl"

layout (location = 0) in vec3 i_FragColor;

layout (location = 0) out vec4 o_Color;

void main() {
   o_Color = vec4(i_FragColor, 1.0);
}