#include "Scene.h"

#include <Application.h>

#include "pch.h"

Scene::Scene(std::shared_ptr<VulkanDevice> device, const std::filesystem::path &scenePath) : m_Device(device) {
    tinygltf::TinyGLTF gltfContext;
    tinygltf::Model glTFInput;
    std::string error, warning;

    bool fileLoaded = false;
    if (scenePath.extension() == ".glb") {
        fileLoaded = gltfContext.LoadBinaryFromFile(&glTFInput, &error, &warning, scenePath.string());
    } else if (scenePath.extension() == ".gltf") {
        fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, scenePath.string());
    }

    if (!fileLoaded) {
        throw std::runtime_error("Could not open " + scenePath.string() + " scene file. Exiting... \n" + warning +
                                 error);
    }

    m_ResourcePath = scenePath.parent_path();

    std::vector<uint32_t> indexBuffer;
    std::vector<Vertex> vertexBuffer;

    LoadImages(glTFInput);
    LoadTextureSamplers(glTFInput);
    LoadTextures(glTFInput);
    LoadMaterials(glTFInput);
    const tinygltf::Scene &scene = glTFInput.scenes[0];
    for (int i: scene.nodes) {
        const tinygltf::Node node = glTFInput.nodes[i];
        LoadNode(glTFInput, node, nullptr, vertexBuffer, indexBuffer);
    }

    CreateVertexBuffer(vertexBuffer);
    CreateIndexBuffer(indexBuffer);
    CreateLights();
}

void Scene::CreateVertexBuffer(std::vector<Vertex> &vertices) {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VulkanBuffer stagingBuffer(m_Device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
                               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                       VMA_ALLOCATION_CREATE_MAPPED_BIT);
    stagingBuffer.From(vertices.data(), (size_t) bufferSize);

    m_VertexBuffer =
            VulkanBuffer(m_Device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VMA_MEMORY_USAGE_AUTO);
    m_VertexBuffer.FromBuffer(stagingBuffer);
    stagingBuffer.Destroy();
}

void Scene::CreateIndexBuffer(std::vector<uint32_t> &indices) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VulkanBuffer stagingBuffer(m_Device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
                               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                       VMA_ALLOCATION_CREATE_MAPPED_BIT);
    stagingBuffer.From(indices.data(), (size_t) bufferSize);

    m_IndexBuffer =
            VulkanBuffer(m_Device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                         VMA_MEMORY_USAGE_AUTO);
    m_IndexBuffer.FromBuffer(stagingBuffer);
    stagingBuffer.Destroy();
}

void Scene::LoadImages(tinygltf::Model &input) {
    m_Images.resize(input.images.size() + 1);
    for (size_t i = 0; i < input.images.size(); i++) {
        tinygltf::Image &glTFImage = input.images[i];

        // Load from disk
        if (!glTFImage.uri.empty()) {
            TextureSpecification spec{.width = static_cast<uint32_t>(glTFImage.width),
                                      .height = static_cast<uint32_t>(glTFImage.height),
                                      .generateMipMaps = true};
            m_Images[i] = std::make_shared<Texture2D>(m_Device, spec, m_ResourcePath / glTFImage.uri);
        } else { // https://github.com/SaschaWillems/Vulkan/blob/master/examples/gltfloading/gltfloading.cpp
            unsigned char *buffer;
            VkDeviceSize bufferSize;
            bool deleteBuffer = false;
            // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
            if (glTFImage.component == 3) {
                bufferSize = glTFImage.width * glTFImage.height * 4;
                buffer = new unsigned char[bufferSize];
                unsigned char *rgba = buffer;
                unsigned char *rgb = &glTFImage.image[0];
                for (size_t j = 0; j < glTFImage.width * glTFImage.height; ++j) {
                    memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                    rgba += 4;
                    rgb += 3;
                }
                deleteBuffer = true;
            } else {
                buffer = &glTFImage.image[0];
                bufferSize = glTFImage.image.size();
            }
            TextureSpecification spec{.width = (uint32_t) glTFImage.width, .height = (uint32_t) glTFImage.height};
            m_Images[i] = std::make_shared<Texture2D>(m_Device, spec, buffer);
            if (deleteBuffer) {
                delete[] buffer;
            }
        }
    }

    // Default image/texture
    std::array<unsigned char, 1 * 1 * 4> pixels = {128, 128, 128, 255};
    TextureSpecification spec{.width = 1, .height = 1};
    m_Images[m_Images.size() - 1] = std::make_shared<Texture2D>(m_Device, spec, pixels.data());
}

void Scene::LoadTextures(tinygltf::Model &input) {
    m_Textures.resize(input.textures.size());
    for (size_t i = 0; i < input.textures.size(); i++) {
        m_Textures[i].imageIndex = input.textures[i].source;
        if (input.textures[i].sampler != -1) {
            TextureSampler sampler = m_TextureSamplers[input.textures[i].sampler];
            m_Images[m_Textures[i].imageIndex]->SetSampler(sampler);
        }
    }

    // TODO: Check better way to do later
    m_DefaultTexture.imageIndex = static_cast<int32_t>(m_Images.size() - 1);
}

static TextureWrapMode GetWrapMode(int32_t wrapMode) {
    switch (wrapMode) {
        case -1:
        case 10497:
            return TextureWrapMode::Repeat;
        case 33071:
            return TextureWrapMode::Clamp;
        case 33648:
            return TextureWrapMode::MirroredRepeat;
        default:
            std::cerr << "Unknown wrap mode for getVkWrapMode: " << wrapMode << std::endl;
            return TextureWrapMode::Repeat;
    }
}

static TextureFilterMode GetFilterMode(int32_t filterMode) {
    switch (filterMode) {
        case -1:
        case 9728:
        case 9984:
        case 9985:
            return TextureFilterMode::Nearest;
        case 9986:
        case 9987:
        case 9729:
            return TextureFilterMode::Linear;
        default:
            std::cerr << "Unknown filter mode for getVkFilterMode: " << filterMode << std::endl;
            return TextureFilterMode::Nearest;
    }
}

void Scene::LoadTextureSamplers(tinygltf::Model &input) {
    for (const tinygltf::Sampler &smpl: input.samplers) {
        TextureSampler sampler{
                .samplerWrap = GetWrapMode(smpl.wrapS),
                .samplerFilter = GetFilterMode(smpl.minFilter),
        };
        m_TextureSamplers.push_back(sampler);
    }
}

void Scene::LoadMaterials(tinygltf::Model &input) {
    m_DefaultMaterial = {};

    m_Materials.resize(input.materials.size());
    for (size_t i = 0; i < input.materials.size(); i++) {
        Material material{}; // Must have a different name than below, or it doesn't work

        tinygltf::Material glTFMaterial = input.materials[i];
        if (glTFMaterial.values.contains("baseColorFactor")) {
            material.baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
        }

        if (glTFMaterial.values.contains("baseColorTexture")) {
            material.baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
            material.baseColorTextureUV = glTFMaterial.values["baseColorTexture"].TextureTexCoord();
        }

        if (glTFMaterial.values.contains("roughnessFactor")) {
            material.roughnessFactor = glm::vec4(static_cast<float>(glTFMaterial.values["roughnessFactor"].Factor()));
        }

        if (glTFMaterial.values.contains("metallicFactor")) {
            material.metallicFactor = glm::vec4(static_cast<float>(glTFMaterial.values["metallicFactor"].Factor()));
        }

        if (glTFMaterial.additionalValues.contains("emissiveFactor")) {
            material.emissiveFactor = glm::vec4(
                    glm::make_vec3(glTFMaterial.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
        }

        if (glTFMaterial.values.contains("metallicRoughnessTexture")) {
            material.metallicRoughnessTextureIndex = glTFMaterial.values["metallicRoughnessTexture"].TextureIndex();
            material.metallicRoughnessTextureUV = glTFMaterial.values["metallicRoughnessTexture"].TextureTexCoord();
        }

        // Get the normal map texture index
        if (glTFMaterial.additionalValues.contains("normalTexture")) {
            material.normalTextureIndex = glTFMaterial.additionalValues["normalTexture"].TextureIndex();
            material.normalTextureUV = glTFMaterial.additionalValues["normalTexture"].TextureTexCoord();
        }

        if (glTFMaterial.additionalValues.contains("emissiveTexture")) {
            material.emissiveTextureIndex = glTFMaterial.additionalValues["emissiveTexture"].TextureIndex();
            material.emissiveTextureUV = glTFMaterial.additionalValues["emissiveTexture"].TextureTexCoord();
        }

        if (glTFMaterial.additionalValues.contains("alphaMode")) {
            tinygltf::Parameter param = glTFMaterial.additionalValues["alphaMode"];
            if (param.string_value == "BLEND") {
                material.alphaMask = 2.0f;
            }
            if (param.string_value == "MASK") {
                material.alphaCutoff = 0.5f;
                material.alphaMask = 1.0f;
            }
        }
        if (glTFMaterial.additionalValues.contains("alphaCutoff")) {
            material.alphaCutoff = static_cast<float>(glTFMaterial.additionalValues["alphaCutoff"].Factor());
        }

        m_Materials[i] = material;
    }
}

void Scene::LoadNode(const tinygltf::Model &input, const tinygltf::Node &inputNode, Scene::Node *parent,
                     std::vector<Vertex> &vertexBuffer, std::vector<uint32_t> &indexBuffer) {
    auto node = new Node{};
    node->matrix = glm::mat4(1.0f);
    node->parent = parent;

    // Get the local node matrix
    // It's either made up from translation, rotation, scale or a 4x4 matrix
    if (inputNode.translation.size() == 3) {
        node->matrix = glm::translate(node->matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
    }
    if (inputNode.rotation.size() == 4) {
        glm::quat q = glm::make_quat(inputNode.rotation.data());
        node->matrix *= glm::mat4(q);
    }
    if (inputNode.scale.size() == 3) {
        node->matrix = glm::scale(node->matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
    }
    if (inputNode.matrix.size() == 16) {
        node->matrix = glm::make_mat4x4(inputNode.matrix.data());
    }

    if (!inputNode.children.empty()) {
        for (int i: inputNode.children) {
            LoadNode(input, input.nodes[i], node, vertexBuffer, indexBuffer);
        }
    }

    // If the node contains mesh data, we load vertices and indices from the buffers
    // In glTF this is done via accessors and buffer views
    if (inputNode.mesh > -1) {
        const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];

        // Iterate through all primitives of this node's mesh
        for (const auto &glTFPrimitive: mesh.primitives) {
            auto firstIndex = static_cast<uint32_t>(indexBuffer.size());
            auto vertexStart = static_cast<uint32_t>(vertexBuffer.size());
            uint32_t indexCount = 0;

            // Vertices
            {
                const float *positionBuffer = nullptr;
                const float *normalsBuffer = nullptr;
                const float *texCoordsBuffer0 = nullptr;
                const float *texCoordsBuffer1 = nullptr;
                const float *colorBuffer = nullptr;
                size_t vertexCount = 0;

                // Get buffer data for vertex positions
                if (glTFPrimitive.attributes.contains("POSITION")) {
                    const tinygltf::Accessor &accessor =
                            input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    positionBuffer = reinterpret_cast<const float *>(
                            &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    vertexCount = accessor.count;
                }
                // Get buffer data for vertex normals
                if (glTFPrimitive.attributes.contains("NORMAL")) {
                    const tinygltf::Accessor &accessor =
                            input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    normalsBuffer = reinterpret_cast<const float *>(
                            &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }
                // Get buffer data for vertex texture coordinates
                // glTF supports multiple sets, we only load the first one
                if (glTFPrimitive.attributes.contains("TEXCOORD_0")) {
                    const tinygltf::Accessor &accessor =
                            input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    texCoordsBuffer0 = reinterpret_cast<const float *>(
                            &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }

                if (glTFPrimitive.attributes.contains("TEXCOORD_1")) {
                    const tinygltf::Accessor &accessor =
                            input.accessors[glTFPrimitive.attributes.find("TEXCOORD_1")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    texCoordsBuffer1 = reinterpret_cast<const float *>(
                            &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }

                // Vertex colors
                if (glTFPrimitive.attributes.contains("COLOR_0")) {
                    const tinygltf::Accessor &accessor =
                            input.accessors[glTFPrimitive.attributes.find("COLOR_0")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    colorBuffer = reinterpret_cast<const float *>(
                            &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                }

                // Append data to model's vertex buffer
                for (size_t v = 0; v < vertexCount; v++) {
                    Vertex vert{};
                    vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                    vert.normal = glm::normalize(
                            glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                    vert.color = colorBuffer ? glm::make_vec4(&colorBuffer[v * 4]) : glm::vec4(1.0f);
                    vert.texCoord0 = texCoordsBuffer0 ? glm::make_vec2(&texCoordsBuffer0[v * 2]) : glm::vec3(0.0f);
                    vert.texCoord1 = texCoordsBuffer1 ? glm::make_vec2(&texCoordsBuffer1[v * 2]) : glm::vec3(0.0f);
                    vertexBuffer.push_back(vert);
                }
            }

            // Indices
            {
                const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.indices];
                const tinygltf::BufferView &bufferView = input.bufferViews[accessor.bufferView];
                const tinygltf::Buffer &buffer = input.buffers[bufferView.buffer];

                indexCount += static_cast<uint32_t>(accessor.count);

                // glTF supports different component types of indices
                switch (accessor.componentType) {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        const auto *buf = reinterpret_cast<const uint32_t *>(
                                &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        const auto *buf = reinterpret_cast<const uint16_t *>(
                                &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        const auto *buf = reinterpret_cast<const uint8_t *>(
                                &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    default:
                        std::cerr << "Index component type " << accessor.componentType << " not supported!"
                                  << std::endl;
                        return;
                }
            }
            Primitive primitive{};
            primitive.firstIndex = firstIndex;
            primitive.indexCount = indexCount;
            primitive.materialIndex = glTFPrimitive.material;
            node->mesh.primitives.push_back(primitive);
        }
    }

    if (parent) {
        parent->children.push_back(node);
    } else {
        m_Nodes.push_back(node);
    }
}

void Scene::Destroy() {
    m_IndexBuffer.Destroy();
    m_VertexBuffer.Destroy();

    for (auto node: m_Nodes) {
        delete node;
    }

    //    m_DefaultImage.texture.Destroy();
    for (auto &image: m_Images) {
        image->Destroy();
    }
}

void Scene::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, bool isSkybox, bool isShadowMap) {
    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {m_VertexBuffer.GetBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    // Render all nodes at top-level
    for (const auto &node: m_Nodes) {
        DrawNode(commandBuffer, pipelineLayout, node, OPAQUE, isSkybox, isShadowMap);
    }

    for (const auto &node: m_Nodes) {
        DrawNode(commandBuffer, pipelineLayout, node, MASK, isSkybox, isShadowMap);
    }
}

void Scene::DrawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Node *node, AlphaMode alphaMode,
                     bool isSkybox, bool isShadowMap) {
    if (!node->mesh.primitives.empty()) {
        // Pass the node's matrix via push constants
        // Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
        // TODO: Inefficient? -> Search for different scene graph implementations
        glm::mat4 nodeMatrix = node->matrix;
        Node *currentParent = node->parent;
        while (currentParent) {
            nodeMatrix = currentParent->matrix * nodeMatrix;
            currentParent = currentParent->parent;
        }
        // TODO Cleanup
        // Pass the final matrix to the vertex shader using push constants
        if (!isSkybox && isShadowMap) {

            struct shadowPushConstants {
                glm::mat4 model;
                VkDeviceAddress lightBufferAddress;
                int32_t directionalLightIndex;
            };

            const auto pushConstants = shadowPushConstants{nodeMatrix, Application::lightsBufferAddress, 0};
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants),
                               &pushConstants);
        }
        for (Primitive &primitive: node->mesh.primitives) {
            if (primitive.indexCount > 0) {
                // Get the texture index for this primitive
                auto &material =
                        primitive.materialIndex != -1 ? m_Materials[primitive.materialIndex] : m_DefaultMaterial;

                if (alphaMode == OPAQUE && material.alphaMask != 0.0f)
                    continue;

                if (alphaMode == MASK && material.alphaMask != 1.0f)
                    continue;


                if (!isSkybox && !isShadowMap) {
                    struct PBRPushConstants {
                        glm::mat4 model;
                        VkDeviceAddress materialsBufferAddress;
                        int32_t materialIndex;
                        VkDeviceAddress lightsBufferAddress;
                        int32_t directionLightIndex;
                        int32_t lightCount;
                        int32_t shadowMapTextureIndex;
                        VkDeviceAddress cameraBufferAddress;
                    };

                    const PBRPushConstants pushConstants = {nodeMatrix,
                                                            Application::materialsBufferAddress,
                                                            primitive.materialIndex,
                                                            Application::lightsBufferAddress,
                                                            0,
                                                            static_cast<uint32_t>(m_Lights.size()),
                                                            800,
                                                            Application::cameraBufferAddress};
                    vkCmdPushConstants(commandBuffer, pipelineLayout,
                                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                       sizeof(PBRPushConstants), &pushConstants);
                }

                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
            }
        }
    }
    for (const auto &child: node->children) {
        DrawNode(commandBuffer, pipelineLayout, child, alphaMode, isSkybox, isShadowMap);
    }
}

void Scene::CreateLights() {
    constexpr auto lightDirection = glm::vec3(0.1f, 1.0f, 0.25f);
    m_Lights.push_back(
            {.direction = lightDirection,
             .view = glm::lookAt(lightDirection * 15.0f, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
             .proj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 100.0f),
             .type = Light::Type::DIRECTIONAL});

    m_Lights.push_back({.position = glm::vec3(-11.0f, 0.1f, -0.3f), .type = Light::Type::POINT});
    m_Lights.push_back({.position = glm::vec3(-6.5f, 1.0f, -1.5f), .type = Light::Type::POINT});
}
