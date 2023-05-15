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

layout (location = 0) in vec3 i_Position;
layout (location = 1) in vec3 i_Normal;
layout (location = 2) in vec2 i_UV;
layout (location = 3) in vec3 i_Color;

layout (location = 0) out vec3 o_Color;
layout (location = 1) out vec3 o_Normal;
layout (location = 2) out vec2 o_UV;
layout (location = 3) out vec3 o_ViewVec;
layout (location = 4) out vec3 o_LightVec;
layout (location = 5) out vec3 o_FragPos;


// https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/pbr.vert
void main() {
    gl_Position = ubo.proj * ubo.view * primitive.model * vec4(i_Position, 1.0);
    o_Color = i_Color;
    o_UV = i_UV;

    //    outNormal = mat3(transpose(inverse(primitive.model))) * inNormal;
    o_Normal = mat3(primitive.model) * i_Normal;
    vec4 pos = primitive.model * vec4(i_Position, 1.0);
    o_LightVec = sceneInfo.lightPos.xyz - pos.xyz;
    o_ViewVec = ubo.viewPos.xyz - pos.xyz;
    o_FragPos = vec3(pos);
}
