#pragma once

#include "Vulkan/VulkanTexture.h"
#include "vulkan/VulkanBuffer.h"

class Scene {
public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec3 color;
    };

    // A primitive contains the data for a single draw call
    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        int32_t materialIndex;
    };

    struct Mesh {
        std::vector<Primitive> primitives;
    };

    struct Material {
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        int32_t baseColorTextureIndex;
    };

    struct Image {
        VulkanTexture texture;
        VkDescriptorSet descriptorSet;
    };

    struct Texture {
        int32_t imageIndex;
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

    Scene() = default;
    Scene(std::shared_ptr<VulkanDevice> device, const std::filesystem::path &filePath);
    void Destroy();

    void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

private:
    void DrawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Node *node);

    void CreateIndexBuffer(std::vector<uint32_t> &indices);
    void CreateVertexBuffer(std::vector<Vertex> &vertices);

    void LoadImages(tinygltf::Model &input);
    void LoadTextures(tinygltf::Model &input);
    void LoadMaterials(tinygltf::Model &input);
    void LoadNode(const tinygltf::Model &input, const tinygltf::Node &inputNode, Node *parent,
                  std::vector<Vertex> &vertexBuffer, std::vector<uint32_t> &indexBuffer);

    std::vector<Texture> m_Textures;
    std::vector<Image> m_Images;
    std::vector<Material> m_Materials;
    std::vector<Node *> m_Nodes;

    Texture m_DefaultTexture;
    Image m_DefaultImage;

    VulkanBuffer m_VertexBuffer;
    VulkanBuffer m_IndexBuffer;

    std::filesystem::path m_ResourcePath;

    std::shared_ptr<VulkanDevice> m_Device;
};
