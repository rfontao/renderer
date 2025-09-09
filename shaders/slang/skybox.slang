struct Textures {
    SamplerCube cube[];
};
ParameterBlock<Textures> textures;

struct Vertex {
    float3 position  : POSITION;
    float3 color     : COLOR;
    float3 normal    : NORMAL;
    float2 texCoord  : TEXCOORD0;
    float2 texCoord1 : TEXCOORD1;
};

struct Camera {
    float4x4 view;
    float4x4 proj;
    float3 position;
};

struct VSOutput {
    float4 position : SV_Position;
    float3 texCoord;
};

struct FSOutput {
    float4 color;
};

struct PushConstants {
    Camera* camerasBuffer;
    int cameraIndex;
    int skyboxTextureIndex;
};


[[vk::push_constant]] uniform PushConstants pc;

[shader("vertex")]
VSOutput vertexMain(Vertex vertex) {
    VSOutput output;

    Camera camera = pc.camerasBuffer[pc.cameraIndex];
    float4x4 viewNoTranslation = camera.view;
    viewNoTranslation[3] = float4(0.0, 0.0, 0.0, camera.view[3].w);
    output.position = mul(camera.proj, mul(viewNoTranslation, float4(vertex.position, 1.0)));
    output.texCoord = vertex.position;

    return output;
}

[shader("fragment")]
FSOutput fragmentMain(VSOutput input) {
    FSOutput output;
    output.color = textures.cube[NonUniformResourceIndex(pc.skyboxTextureIndex)].Sample(input.texCoord);
    return output;
}