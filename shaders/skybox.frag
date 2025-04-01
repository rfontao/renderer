#version 460

layout (set = 0, binding = 0) uniform samplerCube skyboxTexture;

layout (location = 0) in vec3 i_texCoords;

layout (location = 0) out vec4 o_Color;

void main() {
    o_Color = texture(skyboxTexture, i_texCoords);
}