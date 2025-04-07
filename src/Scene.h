#pragma once

#include "Vulkan/Buffer.h"
#include "Vulkan/VulkanTexture.h"

#include <Camera.h>

class Scene {
public:
    enum AlphaMode { OPAQUE, MASK, BLEND };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 texCoord0;
        glm::vec2 texCoord1;
    };

    struct Mesh {
        uint32_t firstIndex{0};
        uint32_t indexCount{0};
        int32_t materialIndex{-1};
    };

    struct Material {
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        glm::vec4 metallicFactor = glm::vec4(1.0f);
        glm::vec4 roughnessFactor = glm::vec4(1.0f);
        glm::vec4 emissiveFactor = glm::vec4(1.0f);

        int32_t baseColorTextureIndex = -1;
        int32_t normalTextureIndex = -1;
        int32_t metallicRoughnessTextureIndex = -1;
        int32_t emissiveTextureIndex = -1;

        int32_t baseColorTextureUV = -1;
        int32_t normalTextureUV = -1;
        int32_t metallicRoughnessTextureUV = -1;
        int32_t emissiveTextureUV = -1;

        float alphaMask = 0.0f;
        float alphaCutoff = 1.0f;
    };

    struct Texture {
        int32_t imageIndex;
    };

    struct Node {
        Node *parent;
        std::vector<Node *> children;
        std::vector<uint32_t> meshIndices;
        uint32_t modelMatrixIndex{0};

        ~Node() {
            for (const auto &child: children) {
                delete child;
            }
        }
    };

    struct Light {
        enum class Type { DIRECTIONAL = 0, POINT = 1 };

        glm::vec3 position{};
        glm::vec3 direction{};
        glm::vec3 color{};
        glm::mat4 view{};
        glm::mat4 proj{};
        float intensity{};
        Type type{};
    };

    Scene() = default;

    Scene(std::shared_ptr<VulkanDevice> device, const std::filesystem::path &scenePath);

    void Destroy();

    void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, bool isSkybox = false,
              bool isShadowMap = false);

    std::vector<Texture> m_Textures;
    std::vector<TextureSampler> m_TextureSamplers;
    std::vector<std::shared_ptr<Texture2D>> m_Images;

    Texture m_DefaultTexture;
    std::shared_ptr<VulkanTexture> m_DefaultImage;
    Material m_DefaultMaterial;

private:
    void DrawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Node *node, AlphaMode alphaMode,
                  bool isSkybox = false, bool isShadowMap = false);

    void CreateIndexBuffer(std::vector<uint32_t> &indices);
    void CreateVertexBuffer(std::vector<Vertex> &vertices);

    void LoadImages(tinygltf::Model &input);
    void LoadTextures(tinygltf::Model &input);
    void LoadTextureSamplers(tinygltf::Model &input);
    void LoadMaterials(tinygltf::Model &input);

    void LoadNode(const tinygltf::Model &input, const tinygltf::Node &inputNode, Node *parent,
                  std::vector<Vertex> &vertexBuffer, std::vector<uint32_t> &indexBuffer);

    void CreateLights();
    void CreateBuffers();

public:
    std::vector<Material> m_Materials;
    std::vector<Node *> m_Nodes;
    std::vector<Mesh> meshes;
    std::vector<glm::mat4> modelMatrices;

    std::vector<Light> m_Lights;

    std::unique_ptr<Buffer> m_VertexBuffer;
    std::unique_ptr<Buffer> m_IndexBuffer;

    std::shared_ptr<Buffer> materialsBuffer;
    std::shared_ptr<Buffer> lightsBuffer;
    std::shared_ptr<Buffer> cameraBuffer;

    std::filesystem::path m_ResourcePath;


    Camera camera;

    std::shared_ptr<VulkanDevice> m_Device;
};
