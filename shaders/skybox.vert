#version 460

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 viewPos;
} ubo;

layout (location = 0) in vec3 i_Position;
layout (location = 1) in vec3 i_Normal;
layout (location = 2) in vec3 i_Color;
layout (location = 3) in vec2 i_UV0;
layout (location = 4) in vec2 i_UV1;

layout (location = 0) out vec3 o_TexCoords;

void main()
{
    o_TexCoords = i_Position;

    mat4 viewNoTranslation = mat4(mat3(ubo.view));
    gl_Position = ubo.proj * viewNoTranslation * vec4(i_Position.xyz, 1.0);
}