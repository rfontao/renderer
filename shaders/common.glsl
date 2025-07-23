#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier: enable

struct Camera {
    mat4 view;
    mat4 proj;
    vec3 position;
};

layout(std430, buffer_reference, buffer_reference_align = 8) buffer CameraBuffer {
    Camera cameras[];
};

struct Material {
    vec4 baseColorFactor;
    vec4 metallicFactor;
    vec4 roughnessFactor;
    vec4 emissiveFactor;

    int baseColorTextureIndex;
    int normalTextureIndex;
    int metallicRoughnessTextureIndex;
    int emissiveTextureIndex;

    int baseColorTextureUV;
    int normalTextureUV;
    int metallicRoughnessTextureUV;
    int emissiveTextureUV;

    float alphaMask;
    float alphaMaskCutoff;
};

layout(std430, buffer_reference, buffer_reference_align = 8) buffer MaterialBuffer {
    Material materials[];
};


struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    mat4 view;
    mat4 proj;
    float intensity;
    int type;
};

layout(std430, buffer_reference, buffer_reference_align = 8) buffer LightsBuffer {
    Light lights[];
};


struct DrawData {
    uint modelMatrixIndex;
    uint materialIndex;
    vec4 boundingSphere;
};

layout(std430, buffer_reference, buffer_reference_align = 8) buffer DrawDataBuffer {
    DrawData drawData[];
};

layout(std430, buffer_reference, buffer_reference_align = 8) buffer ModelMatricesBuffer {
    mat4 matrices[];
};


