#version 460

#include "common.glsl"

layout (set = 0, binding = 0) uniform sampler2D textures2D[];

layout (push_constant, scalar) uniform PushConsts {
    LightsBuffer lightsBufferAddress;
    CameraBuffer cameraBufferAddress;
    int directionLightIndex;
    int lightCount;
    int shadowMapTextureIndex;
    int cameraIndex;
    int colorTextureIndex;
    int normalTextureIndex;
    int metallicRoughnessAOTextureIndex;
} pc;

layout (location = 0) in vec2 i_UV0;

layout (location = 0) out vec4 o_Color;

const float PI = 3.1415926535897932384626433832795;
const int PCF_SIZE = 3;

// https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/pbr_khr.frag
vec4 SRGBtoLINEAR(vec4 srgbIn)
{
    vec3 linOut = pow(srgbIn.xyz, vec3(2.2));
    return vec4(linOut, srgbIn.w);
}

// https://google.github.io/filament/Filament.html#materialsystem/specularbrdf
// Specular D
float NormalDistributionFunction(float NoH, float roughness) {
    float a = NoH * roughness;
    float k = roughness / (1.0 - NoH * NoH + a * a);
    return k * k * (1.0 / PI);
}

// Specular G
float GeometryFunction(float NoL, float NoV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float gl = NoL / (NoL * (1.0 - k) + k);
    float gv = NoV / (NoV * (1.0 - k) + k);

    return gl * gv;
}

// Fresnel
vec3 F_Schlick(float cosTheta, float metallic, vec3 albedo)
{
    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metal-brdf-and-dielectric-brdf
    vec3 F0 = mix(vec3(0.04), albedo, metallic); // * material.specular
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    return F;
}

vec3 BRDF(vec3 L, vec3 V, vec3 N, vec3 radiance, float metallic, float roughness, vec3 albedo)
{
    // Precalculate vectors and dot products
    vec3 H = normalize(V + L);
    float dotNV = clamp(abs(dot(N, V)), 0.001, 1.0);
    float dotNL = clamp(dot(N, L), 0.001, 1.0);
    float dotLH = clamp(dot(L, H), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

    vec3 color = vec3(0.0);
    if (dotNL > 0.0)
    {
        // D = Normal distribution (Distribution of the microfacets)
        float D = NormalDistributionFunction(dotNH, roughness);
        // G = Geometric shadowing term (Microfacets shadowing)
        float G = GeometryFunction(dotNL, dotNV, roughness);
        // F = Fresnel factor (Reflectance depending on angle of incidence)
        vec3 F = F_Schlick(dotNV, metallic, albedo);

        // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#specular-brdf
        vec3 specular = D * F * G / (4.0 * dotNL * dotNV + 0.0001);
        vec3 diffuse = (1.0 - F) * (1.0 / PI) * (1.0 - metallic) * albedo;
        color += (specular + diffuse) * dotNL * radiance;
    }

    return color;
}

float CalculateShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    if (abs(projCoords.x) > 1.0 ||
    abs(projCoords.y) > 1.0 ||
    abs(projCoords.z) > 1.0) {
        return 0.0;
    }

    // https://blogs.igalia.com/itoral/2017/10/02/working-with-lights-and-shadows-part-iii-rendering-the-shadows/
    // Translate from NDC to shadow map space (Vulkan's Z is already in [0..1])
    vec2 shadowMapCoords = projCoords.xy * 0.5 + 0.5;

    // PCF Implementation
    float shadow = 0.0f;
    vec2 shadowMapTexelSize = 1.0f / textureSize(textures2D[nonuniformEXT(pc.shadowMapTextureIndex)], 0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 PCFCoords = shadowMapCoords + vec2(x, y) * shadowMapTexelSize;

            // Check if the sample is in light or in the shadow
            if (projCoords.z <= texture(textures2D[nonuniformEXT(pc.shadowMapTextureIndex)], PCFCoords).r) {
                shadow += 1.0;
            }
        }
    }

    return shadow / 9.0f;
}

void main() {
    DrawData drawData = pc.drawDataBufferAddress.drawData[i_DrawID];
    Material material = pc.materialBufferAddress.materials[drawData.materialIndex];
    Light directionalLight = pc.lightsBufferAddress.lights[pc.directionLightIndex];

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

    const float ambient = 0.1;
    vec3 N = GetNormal();
    vec3 V = normalize(i_ViewVec);

    vec3 Lo = vec3(0.0);
    // Directional light -> Attenuation is 1.0 (no attenuation)
    vec3 L = normalize(directionalLight.direction);
    vec3 radiance = SRGBtoLINEAR(vec4(3.0f)).rgb;
    Lo += BRDF(L, V, N, radiance, metallic, roughness, color.rgb);

    // Point Lights
    for (int i = 0; i < pc.lightCount; i++) {
        Light light = pc.lightsBufferAddress.lights[i];
        // If directionlight, skip
        if (light.type == 0) {
            continue;
        }

        L = normalize(light.position - i_FragPos);
        float distance = length(light.position - i_FragPos);
        float attenuation = 1.0 / (distance * distance);
        radiance = SRGBtoLINEAR(vec4(1.0f)).rgb * attenuation;

        Lo += BRDF(L, V, N, radiance, metallic, roughness, color.rgb);
    }

    float shadow = texture(textures2D[nonuniformEXT(pc.normalTextureIndex)], i_UV0);
    o_Color = shadow * vec4(Lo, 0.0f) + vec4(color.rgb * ambient, color.a);
}