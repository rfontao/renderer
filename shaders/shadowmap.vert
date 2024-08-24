#version 460

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 viewPos;
} light;

layout (push_constant) uniform PushConsts {
    mat4 model;
} primitive;

layout (location = 0) in vec3 i_Position;
layout (location = 1) in vec3 i_Normal;
layout (location = 2) in vec3 i_Color;
layout (location = 3) in vec2 i_UV0;
layout (location = 4) in vec2 i_UV1;

// https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/pbr.vert
void main() {
    gl_Position = light.proj * light.view * primitive.model * vec4(i_Position, 1.0);
}
