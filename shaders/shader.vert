#version 460

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout (push_constant) uniform PushConsts {
    mat4 model;
} primitive;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inColor;

layout (location = 0) out vec3 outFragColor;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outFragTexCoord;


void main() {
    gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition, 1.0);
    outFragColor = inColor;
    outNormal = mat3(transpose(inverse(ubo.view * primitive.model))) * inNormal;
    outFragTexCoord = inTexCoord;
}
