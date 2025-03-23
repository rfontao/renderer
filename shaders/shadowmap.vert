#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

struct Light
{
    mat4 view;
    mat4 proj;
    vec4 viewPos;
};

layout(std430, buffer_reference, buffer_reference_align = 64) buffer PointerToLight
{
    Light light;
};

layout (push_constant) uniform PushConsts {
    mat4 model;
    PointerToLight lightBufferAddress;
} pc;

layout (location = 0) in vec3 i_Position;
layout (location = 1) in vec3 i_Normal;
layout (location = 2) in vec3 i_Color;
layout (location = 3) in vec2 i_UV0;
layout (location = 4) in vec2 i_UV1;

// https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/pbr.vert
void main() {
    Light light = pc.lightBufferAddress.light;
    gl_Position = light.proj * light.view * pc.model * vec4(i_Position, 1.0);
}
