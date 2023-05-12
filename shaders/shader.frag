#version 460

layout (set = 1, binding = 0) uniform PerScene {
    vec3 lightPos;
} sceneInfo;

layout (set = 2, binding = 0) uniform sampler2D baseColorTexture;

layout (set = 2, binding = 1) uniform Material {
    vec4 baseColorFactor;
    int baseColorTextureSet;
} material;

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

void main() {
    float ambientStrength = 0.1f;
    vec3 ambient = ambientStrength * vec3(1.0f);

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(sceneInfo.lightPos);
//    vec3 lightDir = normalize(LightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0f, 1.0f, 1.0f);

    vec4 color = material.baseColorFactor * vec4(ambient + diffuse, 1.0f);
    outColor = color * vec4(fragColor, 1.0f) * texture(baseColorTexture, fragTexCoord);
}