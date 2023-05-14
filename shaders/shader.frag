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
} material;

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 fragTexCoord;
layout (location = 3) in vec3 viewVec;
layout (location = 4) in vec3 lightVec;
layout (location = 5) in vec4 tangent;
layout (location = 6) in vec3 fragPos;


layout (location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(baseColorTexture, fragTexCoord) * vec4(fragColor, 1.0f) * material.baseColorFactor;

    vec3 N = normalize(normal);

    if (material.normalTextureSet != -1) {

        vec3 q1 = dFdx(fragPos);
        vec3 q2 = dFdy(fragPos);
        vec2 st1 = dFdx(fragTexCoord);
        vec2 st2 = dFdy(fragTexCoord);

        vec3 T = normalize(q1 * st2.t - q2 * st1.t);
        vec3 B = -normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);


        //        vec3 T = normalize(tangent.xyz);
        //        vec3 B = cross(normal, tangent.xyz) * tangent.w;
        //        mat3 TBN = mat3(T, B, N);
        N = TBN * normalize(texture(normalMapTexture, fragTexCoord).xyz * 2.0 - vec3(1.0));
        //        outColor = vec4(N, 1.0f);
        //        N = normalize(texture(normalMapTexture, fragTexCoord).xyz);
        //        outColor = vec4(N, 1.0f);
    }

    const float ambient = 0.1;
    vec3 L = normalize(lightVec);
    vec3 V = normalize(viewVec);
    vec3 R = reflect(-L, N);
    vec3 diffuse = max(dot(N, L), ambient).rrr;
    float specular = pow(max(dot(R, V), 0.0), 32.0);

    outColor = vec4(diffuse * color.rgb + specular, color.a);

    //    float ambientStrength = 0.1f;
    //    vec3 ambient = ambientStrength * vec3(1.0f);
    //
    //    vec3 norm = normalize(normal);
    //    vec3 lightDir = normalize(sceneInfo.lightPos);
    //    //    vec3 lightDir = normalize(LightPos - FragPos);
    //    float diff = max(dot(norm, lightDir), 0.0);
    //    vec3 diffuse = diff * vec3(1.0f, 1.0f, 1.0f);

    //    vec4 color = material.baseColorFactor * vec4(ambient + diffuse, 1.0f);
    //    outColor = color * vec4(fragColor, 1.0f) * texture(baseColorTexture, fragTexCoord);
}