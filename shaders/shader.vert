#version 460

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 viewPos;
} ubo;

layout (set = 1, binding = 0) uniform PerScene {
    vec3 lightPos;
} sceneInfo;

layout (push_constant) uniform PushConsts {
    mat4 model;
} primitive;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inColor;
layout (location = 4) in vec4 inTangent;

layout (location = 0) out vec3 outFragColor;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outFragTexCoord;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;
layout (location = 5) out vec4 outTangent;
layout (location = 6) out vec3 outFragPos;

void main() {
    gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition, 1.0);
    outFragColor = inColor;
    outFragTexCoord = inTexCoord;
    outTangent = inTangent;

    outNormal = mat3(transpose(inverse(primitive.model))) * inNormal;
    //    outNormal = mat3(primitive.model) * inNormal;
    vec4 pos = primitive.model * vec4(inPosition, 1.0);
    outLightVec = sceneInfo.lightPos.xyz - pos.xyz;
    outViewVec = ubo.viewPos.xyz - pos.xyz;
    outFragPos = vec3(pos);
}
