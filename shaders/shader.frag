#version 460

layout (set = 1, binding = 0) uniform PerScene {
    vec3 lightPos[4];
    int lightCount;
} sceneInfo;

layout (set = 2, binding = 0) uniform sampler2D baseColorTexture;
layout (set = 2, binding = 1) uniform sampler2D normalMapTexture;
layout (set = 2, binding = 2) uniform sampler2D metallicRoughnessTexture;

layout (set = 2, binding = 3) uniform Material {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    int baseColorTextureSet;
    int normalTextureSet;
    int metallicRoughnessTextureSet;
    int baseColorTextureUV;
    int normalTextureUV;
    int metallicRoughnessTextureUV;
} material;

layout (location = 0) in vec3 i_FragColor;
layout (location = 1) in vec3 i_Mormal;
layout (location = 2) in vec2 i_UV0;
layout (location = 3) in vec2 i_UV1;
layout (location = 4) in vec3 i_ViewVec;
layout (location = 5) in vec3 i_FragPos;

layout (location = 0) out vec4 o_Color;

const float PI = 3.1415926535897932384626433832795;

vec3 GetNormal() {
    vec3 N = normalize(i_Mormal);

    if (material.normalTextureSet != -1) {
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

        N = TBN * normalize(texture(normalMapTexture, normalUV).xyz * 2.0 - 1.0);
    }

    return N;
}

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

void main() {


    // White light

    vec2 colorUV = material.baseColorTextureUV == 0 ? i_UV0 : i_UV1;
    vec4 color = SRGBtoLINEAR(texture(baseColorTexture, colorUV)) * vec4(i_FragColor, 1.0f) * material.baseColorFactor;

    float metallic = material.metallicFactor;
    float roughness = material.roughnessFactor;
    if (material.metallicRoughnessTextureSet != -1) {
        vec2 metallicRoughnessUV = material.metallicRoughnessTextureUV == 0 ? i_UV0 : i_UV1;
        // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metallic-roughness-material
        metallic = metallic * texture(metallicRoughnessTexture, metallicRoughnessUV).b;
        roughness = roughness * texture(metallicRoughnessTexture, metallicRoughnessUV).g;
    }

    const float ambient = 0.02;
    vec3 N = GetNormal();
    vec3 V = normalize(i_ViewVec);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < sceneInfo.lightCount; i++) {
        vec3 L = normalize(sceneInfo.lightPos[i] - i_FragPos);
        float distance = length(sceneInfo.lightPos[i] - i_FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = SRGBtoLINEAR(vec4(1.0f)).rgb * attenuation;

        Lo += BRDF(L, V, N, radiance, metallic, roughness, color.rgb);
    }

    o_Color = vec4(Lo, 1.0f) + color * ambient;

    //    const float ambient = 0.1;
    //
    //    vec3 R = reflect(-L, N);
    //    vec3 diffuse = max(dot(N, L), ambient).rrr;
    //    float specular = pow(max(dot(R, V), 0.0), 32.0);
    //
    //    outColor = vec4(diffuse * color.rgb + specular, color.a);
}