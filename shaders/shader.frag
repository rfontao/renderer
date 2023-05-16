#version 460

//layout (set = 1, binding = 0) uniform PerScene {
//    vec3 lightPos;
//} sceneInfo;

layout (set = 2, binding = 0) uniform sampler2D baseColorTexture;
layout (set = 2, binding = 1) uniform sampler2D normalMapTexture;

layout (set = 2, binding = 2) uniform Material {
    vec4 baseColorFactor;
    int baseColorTextureSet;
    int normalTextureSet;

    int baseColorTextureUV;
    int normalTextureUV;
} material;

layout (location = 0) in vec3 i_FragColor;
layout (location = 1) in vec3 i_Mormal;
layout (location = 2) in vec2 i_UV0;
layout (location = 3) in vec2 i_UV1;
layout (location = 4) in vec3 i_ViewVec;
layout (location = 5) in vec3 i_LightVec;
layout (location = 6) in vec3 i_FragPos;

layout (location = 0) out vec4 outColor;

void main() {

    vec2 colorUV = material.baseColorTextureUV == 0 ? i_UV0 : i_UV1;
    vec4 color = texture(baseColorTexture, colorUV) * vec4(i_FragColor, 1.0f) * material.baseColorFactor;

    vec3 N = normalize(i_Mormal);

    if (material.normalTextureSet != -1) {

        vec2 normalUV = material.normalTextureUV == 0 ? i_UV0 : i_UV1;
        // https://github.com/KhronosGroup/Vulkan-Samples/blob/main/shaders/pbr.frag
        vec3 q1 = dFdx(i_FragPos);
        vec3 q2 = dFdy(i_FragPos);
        vec2 st1 = dFdx(normalUV);
        vec2 st2 = dFdy(normalUV);

        vec3 T = (q1 * st2.t - q2 * st1.t) / (st1.s * st2.t - st2.s * st1.t);
        T = normalize(T - N * dot(N, T));
        vec3 B = normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);

        N = TBN * normalize(texture(normalMapTexture, normalUV).xyz * 2.0 - 1.0);
        outColor = vec4(N, 1.0f);
    }

    const float ambient = 0.1;
    vec3 L = normalize(i_LightVec);
    vec3 V = normalize(i_ViewVec);
    vec3 R = reflect(-L, N);
    vec3 diffuse = max(dot(N, L), ambient).rrr;
    float specular = pow(max(dot(R, V), 0.0), 32.0);

    outColor = vec4(diffuse * color.rgb + specular, color.a);
}