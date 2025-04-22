#version 460

#include "common.glsl"

layout (set = 0, binding = 0) uniform samplerCube texturesCube[];

layout (push_constant, scalar) uniform PushConsts {
    CameraBuffer cameraBufferAddress;
    int cameraIndex;
    int skyboxTextureIndex;
    int padding;
} pc;

layout (location = 0) in vec3 i_texCoords;

layout (location = 0) out vec4 o_Color;

void main() {
    o_Color = texture(texturesCube[nonuniformEXT(pc.skyboxTextureIndex)], i_texCoords);
}