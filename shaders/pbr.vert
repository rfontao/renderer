#version 460

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 viewPos;
} ubo;

layout (set = 2, binding = 0) uniform PerScene {
    vec3 lightDir;
    vec3 lightPos[128];
    int lightCount;
    int shadowMapTextureIndex;
    mat4 lightView;
    mat4 lightProj;
} sceneInfo;

layout (push_constant) uniform PushConsts {
    mat4 model;
} primitive;

layout (location = 0) in vec3 i_Position;
layout (location = 1) in vec3 i_Normal;
layout (location = 2) in vec3 i_Color;
layout (location = 3) in vec2 i_UV0;
layout (location = 4) in vec2 i_UV1;

layout (location = 0) out vec3 o_Color;
layout (location = 1) out vec3 o_Normal;
layout (location = 2) out vec2 o_UV0;
layout (location = 3) out vec2 o_UV1;
layout (location = 4) out vec3 o_ViewVec;
layout (location = 5) out vec3 o_FragPos;
layout (location = 6) out vec4 o_FragPosLightSpace;

// https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/pbr.vert
void main() {
    gl_Position = ubo.proj * ubo.view * primitive.model * vec4(i_Position, 1.0);
    o_Color = i_Color;
    o_UV0 = i_UV0;
    o_UV1 = i_UV1;

    o_Normal = normalize(transpose(inverse(mat3(primitive.model))) * i_Normal);
    //    o_Normal = mat3(transpose(inverse(primitive.model))) * i_Normal;
    //    o_Normal = mat3(primitive.model) * i_Normal;

    o_FragPos = vec3(primitive.model * vec4(i_Position, 1.0));
    o_ViewVec = ubo.viewPos.xyz - o_FragPos;
    o_FragPosLightSpace = sceneInfo.lightProj * sceneInfo.lightView * primitive.model * vec4(i_Position, 1.0);
}
