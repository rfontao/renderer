#pragma once

#include "Vulkan/VulkanTexture.h"
#include "vulkan/VulkanBuffer.h"

class Scene {
public:

    enum AlphaMode {
        OPAQUE,
        MASK,
        BLEND
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 texCoord0;
        glm::vec2 texCoord1;
    };

    // A primitive contains the data for a single draw call
    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        int32_t materialIndex = -1;
    };

    struct Mesh {
        std::vector<Primitive> primitives;
    };

    struct MaterialUBO {
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

    struct Material {
        VulkanBuffer info;
        MaterialUBO ubo;
        VkDescriptorSet descriptorSet;
    };

    struct Image {
        VulkanTexture texture;
    };

    struct Texture {
        int32_t imageIndex;
    };

    struct TextureSampler {
        VkFilter magFilter;
        VkFilter minFilter;
        VkSamplerAddressMode addressModeU;
        VkSamplerAddressMode addressModeV;
        VkSamplerAddressMode addressModeW;
    };

    struct Node {
        Node *parent;
        std::vector<Node *> children;
        Mesh mesh;
        glm::mat4 matrix;

        ~Node() {
            for (auto &child: children) {
                delete child;
            }
        }
    };

    struct SceneInfo {
        glm::vec3 lightDir; // 1 directional light
        glm::vec3 lightPos[128];
        int32_t lightCount;
    };

    Scene() = default;
    Scene(std::shared_ptr<VulkanDevice> device, const std::filesystem::path &scenePath);
    void Destroy();

    void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, bool isSkybox = false);
    std::vector<Texture> m_Textures;
    std::vector<TextureSampler> m_TextureSamplers;
    std::vector<Image> m_Images;

    Texture m_DefaultTexture;
    Image m_DefaultImage;
    Material m_DefaultMaterial;

private:
    void DrawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Node *node, AlphaMode alphaMode, bool isSkybox = false);

    void CreateIndexBuffer(std::vector<uint32_t> &indices);
    void CreateVertexBuffer(std::vector<Vertex> &vertices);

    void LoadImages(tinygltf::Model &input);
    void LoadTextures(tinygltf::Model &input);
    void LoadTextureSamplers(tinygltf::Model &input);
    void LoadMaterials(tinygltf::Model &input);
    void LoadNode(const tinygltf::Model &input, const tinygltf::Node &inputNode, Node *parent,
                  std::vector<Vertex> &vertexBuffer, std::vector<uint32_t> &indexBuffer);
    void CreateLights();

    std::vector<Material> m_Materials;
    std::vector<Node *> m_Nodes;



    VulkanBuffer m_VertexBuffer;
    VulkanBuffer m_IndexBuffer;

    VulkanBuffer m_SceneInfo;
    VkDescriptorSet m_SceneInfoDescriptorSet;

    std::filesystem::path m_ResourcePath;

    std::shared_ptr<VulkanDevice> m_Device;
};
