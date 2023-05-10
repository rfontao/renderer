#version 460

layout (set = 1, binding = 0) uniform sampler2D baseColorTexture;

layout (set = 1, binding = 1) uniform Material {
    vec4 baseColorFactor;
    int baseColorTextureSet;
} material;

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = material.baseColorFactor * vec4(fragColor, 1.0f) * texture(baseColorTexture, fragTexCoord);
}