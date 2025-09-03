#version 460

#include "common.glsl"

layout (set = 0, binding = 0) uniform sampler2D textures2D[];

layout (push_constant, scalar) uniform PushConsts {
    MaterialBuffer materialBufferAddress;
    CameraBuffer cameraBufferAddress;
    DrawDataBuffer drawDataBufferAddress;
    ModelMatricesBuffer modelMatricesBufferAddress;
    int cameraIndex;
} pc;

layout (location = 0) in vec3 i_FragColor;
layout (location = 1) in vec3 i_Mormal;
layout (location = 2) in vec2 i_UV0;
layout (location = 3) in vec2 i_UV1;
layout (location = 5) in vec3 i_FragPos;
layout (location = 7) flat in int i_DrawID;

layout (location = 0) out vec4 o_Color;
layout (location = 1) out vec4 o_Normal;
layout (location = 2) out vec4 o_MetallicRoughnessAO;

const float PI = 3.1415926535897932384626433832795;

vec3 GetNormal() {
    DrawData drawData = pc.drawDataBufferAddress.drawData[i_DrawID];
    Material material = pc.materialBufferAddress.materials[drawData.materialIndex];
    vec3 N = normalize(i_Mormal);

    if (material.normalTextureIndex != -1) {
        vec2 normalUV = material.normalTextureUV == 0 ? i_UV0 : i_UV1;
        // https://github.com/KhronosGroup/Vulkan-Samples/blob/main/shaders/pbr.frag
        vec3 q1 = dFdx(i_FragPos);
        vec3 q2 = dFdy(i_FragPos);
        vec2 st1 = dFdx(normalUV);
        vec2 st2 = dFdy(normalUV);

        // Makes sponza not work correctly
        //        vec3 T = (q1 * st2.t - q2 * st1.t) / (st1.s * st2.t - st2.s * st1.t);
        //        T = normalize(T - N * dot(N, T));
        //        vec3 B = normalize(cross(N, T));
        //        mat3 TBN = mat3(T, B, N);

        // Makes helmet not look correctly
        vec3 T = normalize(q1 * st2.t - q2 * st1.t);
        vec3 B = -normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);

        N = TBN * normalize(texture(textures2D[nonuniformEXT(material.normalTextureIndex)], normalUV).xyz * 2.0 - 1.0);
    }

    return N;
}

void main() {
    DrawData drawData = pc.drawDataBufferAddress.drawData[i_DrawID];
    Material material = pc.materialBufferAddress.materials[drawData.materialIndex];

    vec4 color;
    if (material.alphaMask == 1.0f || material.alphaMask == 0.0f) {
        vec2 colorUV = material.baseColorTextureUV == 0 ? i_UV0 : i_UV1;
        color = SRGBtoLINEAR(texture(textures2D[nonuniformEXT(material.baseColorTextureIndex)], colorUV)) * vec4(i_FragColor, 1.0f) * material.baseColorFactor;

        if (material.alphaMask == 1.0f) {
            if (color.a < material.alphaMaskCutoff) {
                discard;
            }
        } else {
            color.a = 1.0f;
        }
    }

    float metallic = material.metallicFactor.x;
    float roughness = material.roughnessFactor.x;
    if (material.metallicRoughnessTextureIndex != -1) {
        vec2 metallicRoughnessUV = material.metallicRoughnessTextureUV == 0 ? i_UV0 : i_UV1;
        // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metallic-roughness-material
        metallic = metallic * texture(textures2D[nonuniformEXT(material.metallicRoughnessTextureIndex)], metallicRoughnessUV).b;
        roughness = roughness * texture(textures2D[nonuniformEXT(material.metallicRoughnessTextureIndex)], metallicRoughnessUV).g;
    }


    o_Color = color;
    o_Normal = GetNormal();
    o_MetallicRoughnessAO = vec3(metallic, roughness, 1.0);
}