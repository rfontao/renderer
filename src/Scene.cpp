#include "Scene.h"

#include <Application.h>

#include <utility>

#include "pch.h"

Scene::Scene(std::shared_ptr<VulkanDevice> device, const std::filesystem::path &scenePath,
             std::shared_ptr<TextureCube> skyboxTexture) :
    m_SkyboxTexture(std::move(skyboxTexture)), m_Device(std::move(device)) {

    cameras.resize(2);
    cameras[0] = Camera(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                        (double) 1280 / (double) 720);

    // NOTE: Debug camera
    cameras[1] = Camera(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                        (double) 1280 / (double) 720);

    UpdateCameraDatas();

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

    GenerateDrawCommands();
    CreateBuffers();
}

void Scene::CreateVertexBuffer(std::vector<Vertex> &vertices) {
    const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    const auto stagingBuffer =
            std::make_unique<Buffer>(m_Device, BufferSpecification{.size = bufferSize, .type = BufferType::STAGING});
    stagingBuffer->From(vertices.data(), bufferSize);

    m_VertexBuffer =
            std::make_unique<Buffer>(m_Device, BufferSpecification{.size = bufferSize, .type = BufferType::VERTEX});
    m_VertexBuffer->FromBuffer(stagingBuffer.get());
    stagingBuffer->Destroy();

    std::vector<Vertex> skyboxVertices = {
            // Front face
            {{-1.0f, -1.0f, 1.0f}},
            {{1.0f, -1.0f, 1.0f}},
            {{1.0f, 1.0f, 1.0f}},
            {{1.0f, 1.0f, 1.0f}},
            {{-1.0f, 1.0f, 1.0f}},
            {{-1.0f, -1.0f, 1.0f}},

            // Back face
            {{-1.0f, -1.0f, -1.0f}},
            {{1.0f, 1.0f, -1.0f}},
            {{1.0f, -1.0f, -1.0f}},
            {{1.0f, 1.0f, -1.0f}},
            {{-1.0f, -1.0f, -1.0f}},
            {{-1.0f, 1.0f, -1.0f}},

            // Left face
            {{-1.0f, -1.0f, -1.0f}},
            {{-1.0f, -1.0f, 1.0f}},
            {{-1.0f, 1.0f, 1.0f}},
            {{-1.0f, 1.0f, 1.0f}},
            {{-1.0f, 1.0f, -1.0f}},
            {{-1.0f, -1.0f, -1.0f}},

            // Right face
            {{1.0f, -1.0f, 1.0f}},
            {{1.0f, -1.0f, -1.0f}},
            {{1.0f, 1.0f, -1.0f}},
            {{1.0f, 1.0f, -1.0f}},
            {{1.0f, 1.0f, 1.0f}},
            {{1.0f, -1.0f, 1.0f}},

            // Top face
            {{-1.0f, 1.0f, 1.0f}},
            {{1.0f, 1.0f, 1.0f}},
            {{1.0f, 1.0f, -1.0f}},
            {{1.0f, 1.0f, -1.0f}},
            {{-1.0f, 1.0f, -1.0f}},
            {{-1.0f, 1.0f, 1.0f}},

            // Bottom face
            {{-1.0f, -1.0f, -1.0f}},
            {{1.0f, -1.0f, -1.0f}},
            {{1.0f, -1.0f, 1.0f}},
            {{1.0f, -1.0f, 1.0f}},
            {{-1.0f, -1.0f, 1.0f}},
            {{-1.0f, -1.0f, -1.0f}},
    };

    const VkDeviceSize skyboxBufferSize = sizeof(skyboxVertices[0]) * skyboxVertices.size();

    const auto skyboxStagingBuffer = std::make_unique<Buffer>(
            m_Device, BufferSpecification{.size = skyboxBufferSize, .type = BufferType::STAGING});
    skyboxStagingBuffer->From(skyboxVertices.data(), skyboxBufferSize);

    m_SkyboxVertexBuffer = std::make_unique<Buffer>(
            m_Device, BufferSpecification{.size = skyboxBufferSize, .type = BufferType::VERTEX});
    m_SkyboxVertexBuffer->FromBuffer(skyboxStagingBuffer.get());
    skyboxStagingBuffer->Destroy();

    constexpr VkDeviceSize frustumBufferSize = sizeof(glm::vec3) * 8;
    m_frustumVertexBuffer = std::make_unique<Buffer>(
            m_Device, BufferSpecification{.size = frustumBufferSize, .type = BufferType::VERTEX});
}

void Scene::CreateIndexBuffer(std::vector<uint32_t> &indices) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    const auto stagingBuffer =
            std::make_unique<Buffer>(m_Device, BufferSpecification{.size = bufferSize, .type = BufferType::STAGING});
    stagingBuffer->From(indices.data(), bufferSize);

    m_IndexBuffer =
            std::make_unique<Buffer>(m_Device, BufferSpecification{.size = bufferSize, .type = BufferType::INDEX});
    m_IndexBuffer->FromBuffer(stagingBuffer.get());
    stagingBuffer->Destroy();


    std::vector<int32_t> frustumIndices = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
    VkDeviceSize frustumIndexBufferSize = frustumIndices.size() * sizeof(int32_t);

    const auto frustumStagingBuffer = std::make_unique<Buffer>(
            m_Device, BufferSpecification{.size = frustumIndexBufferSize, .type = BufferType::STAGING});
    frustumStagingBuffer->From(frustumIndices.data(), frustumIndexBufferSize);
    m_frustumIndexBuffer = std::make_unique<Buffer>(
            m_Device, BufferSpecification{.size = frustumIndexBufferSize, .type = BufferType::INDEX});
    m_frustumIndexBuffer->FromBuffer(frustumStagingBuffer.get());
    frustumStagingBuffer->Destroy();
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
    node->parent = parent;

    // Get the local node matrix
    // It's either made up from translation, rotation, scale or a 4x4 matrix
    auto modelMatrix = glm::mat4(1.0f);
    if (inputNode.translation.size() == 3) {
        modelMatrix = glm::translate(modelMatrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
    }
    if (inputNode.rotation.size() == 4) {
        glm::quat q = glm::make_quat(inputNode.rotation.data());
        modelMatrix *= glm::mat4(q);
    }
    if (inputNode.scale.size() == 3) {
        modelMatrix = glm::scale(modelMatrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
    }
    if (inputNode.matrix.size() == 16) {
        modelMatrix = glm::make_mat4x4(inputNode.matrix.data());
    }

    localModelMatrices.push_back(modelMatrix);
    node->modelMatrixIndex = localModelMatrices.size() - 1;

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

            glm::vec4 boundingSphere{0.0f};

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

                    auto min = glm::vec3{accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]};
                    auto max = glm::vec3{accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]};
                    float sphereRadius = glm::distance(min, max) / 2.0f;
                    boundingSphere = glm::vec4((min + max) / 2.0f, sphereRadius);
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
            Mesh mesh{};
            mesh.firstIndex = firstIndex;
            mesh.indexCount = indexCount;
            mesh.materialIndex = glTFPrimitive.material;
            mesh.boundingSphere = boundingSphere;

            meshes.push_back(mesh);
            node->meshIndices.push_back(meshes.size() - 1);
        }
    }

    if (parent) {
        parent->children.push_back(node);
    } else {
        m_Nodes.push_back(node);
    }
}

void Scene::Destroy() {
    m_IndexBuffer->Destroy();
    m_VertexBuffer->Destroy();

    materialsBuffer->Destroy();
    lightsBuffer->Destroy();
    camerasBuffer->Destroy();

    for (const auto &node: m_Nodes) {
        delete node;
    }

    //    m_DefaultImage.texture.Destroy();
    for (auto &image: m_Images) {
        image->Destroy();
    }
}

void Scene::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const {
    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {m_VertexBuffer->GetBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    struct PBRPushConstants {
        VkDeviceAddress materialsBufferAddress;
        VkDeviceAddress lightsBufferAddress;
        VkDeviceAddress cameraBufferAddress;
        VkDeviceAddress drawDataBufferAddress;
        VkDeviceAddress modelMatricesBufferAddress;
        int32_t directionLightIndex;
        uint32_t lightCount;
        int32_t shadowMapTextureIndex;
        int32_t cameraIndex;
    };

    PBRPushConstants pushConstants = {materialsBuffer->GetAddress(),
                                      lightsBuffer->GetAddress(),
                                      camerasBuffer->GetAddress(),
                                      opaqueDrawDataBuffer->GetAddress(),
                                      modelMatricesBuffer->GetAddress(),
                                      0,
                                      static_cast<uint32_t>(m_Lights.size()),
                                      800,
                                      Application::cameraIndexDrawing};
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(PBRPushConstants), &pushConstants);
    vkCmdDrawIndexedIndirect(commandBuffer, opaqueDrawIndirectCommandsBuffer->GetBuffer(), 0,
                             opaqueDrawIndirectCommands.size(), sizeof(VkDrawIndexedIndirectCommand));

    pushConstants.drawDataBufferAddress = transparentDrawDataBuffer->GetAddress();
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(PBRPushConstants), &pushConstants);
    vkCmdDrawIndexedIndirect(commandBuffer, transparentDrawIndirectCommandsBuffer->GetBuffer(), 0,
                             transparentDrawIndirectCommands.size(), sizeof(VkDrawIndexedIndirectCommand));
}

void Scene::DrawShadowMap(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const {
    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {m_VertexBuffer->GetBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    struct shadowPushConstants {
        VkDeviceAddress lightBufferAddress;
        VkDeviceAddress drawDataBufferAddress;
        VkDeviceAddress modelMatricesBufferAddress;
        int32_t directionalLightIndex;
    };

    auto pushConstants = shadowPushConstants{lightsBuffer->GetAddress(), opaqueDrawDataBuffer->GetAddress(),
                                             modelMatricesBuffer->GetAddress(), 0};
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants),
                       &pushConstants);
    vkCmdDrawIndexedIndirect(commandBuffer, opaqueDrawIndirectCommandsBuffer->GetBuffer(), 0,
                             opaqueDrawIndirectCommands.size(), sizeof(VkDrawIndexedIndirectCommand));

    pushConstants.drawDataBufferAddress = transparentDrawDataBuffer->GetAddress();
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(shadowPushConstants),
                       &pushConstants);
    vkCmdDrawIndexedIndirect(commandBuffer, transparentDrawIndirectCommandsBuffer->GetBuffer(), 0,
                             transparentDrawIndirectCommands.size(), sizeof(VkDrawIndexedIndirectCommand));
}
void Scene::DrawDebugFrustum(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                             uint32_t cameraIndex) const {
    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {m_frustumVertexBuffer->GetBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_frustumIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    struct frustumPushConstants {
        glm::vec3 color;
        VkDeviceAddress cameraBufferAddress;
        uint32_t cameraIndex;
    };

    auto pushConstants = frustumPushConstants{{1.0f, 0.0f, 0.0f}, camerasBuffer->GetAddress(), cameraIndex};
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants),
                       &pushConstants);
    // Draw the frustum
    vkCmdDrawIndexed(commandBuffer, 24, 1, 0, 0, 0);
}

void Scene::DrawSkybox(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {

    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {m_SkyboxVertexBuffer->GetBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    struct SkyboxPushConstant {
        VkDeviceAddress cameraBufferAddress;
        uint32_t cameraIndex;
        uint32_t skyboxTextureIndex;
    };

    SkyboxPushConstant pushConstant{camerasBuffer->GetAddress(), Application::cameraIndexDrawing, 750};
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(SkyboxPushConstant), &pushConstant);
    vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void Scene::GenerateDrawCommands(bool frustumCulling) {
    globalModelMatrices.resize(localModelMatrices.size());
    opaqueDrawData.clear();
    transparentDrawData.clear();
    opaqueDrawIndirectCommands.clear();
    transparentDrawIndirectCommands.clear();

    // Render all nodes at top-level
    for (const auto &node: m_Nodes) {
        DrawNode(node, frustumCulling);
    }
}

void Scene::UpdateCameraDatas() {
    cameraDatas.resize(cameras.size());

    for (size_t i = 0; i < cameras.size(); i++) {
        cameraDatas[i] = cameras[i].GetCameraData();
    }
}

void Scene::DrawNode(Node *node, bool frustumCulling) {
    if (!node->meshIndices.empty()) {
        // Pass the node's matrix via push constants
        // Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
        // TODO: Inefficient? -> Search for different scene graph implementations
        glm::mat4 nodeMatrix = localModelMatrices.at(node->modelMatrixIndex);
        Node *currentParent = node->parent;
        while (currentParent) {
            glm::mat4 parentMatrix = localModelMatrices.at(currentParent->modelMatrixIndex);
            nodeMatrix = parentMatrix * nodeMatrix;
            currentParent = currentParent->parent;
        }
        globalModelMatrices.at(node->modelMatrixIndex) = nodeMatrix;

        for (const auto meshIndex: node->meshIndices) {
            const auto &mesh = meshes[meshIndex];
            if (mesh.indexCount > 0) {

                Camera &camera = cameras[0];
                if (frustumCulling) {
                    auto boundingSphereCenter = glm::vec3(mesh.boundingSphere);
                    auto transformedBoundingSphere = glm::vec3(nodeMatrix * glm::vec4(boundingSphereCenter, 1.0f));
                    if (!camera.DoesSphereIntersectFrustum(
                                glm::vec4(transformedBoundingSphere, mesh.boundingSphere.w))) {
                        continue;
                    }
                }

                // Get the texture index for this primitive
                const auto &material = mesh.materialIndex != -1 ? m_Materials[mesh.materialIndex] : m_DefaultMaterial;

                VkDrawIndexedIndirectCommand drawIndirectCommand{
                        .indexCount = mesh.indexCount,
                        .instanceCount = 1,
                        .firstIndex = mesh.firstIndex,
                        .vertexOffset = 0,
                        .firstInstance = 0,
                };
                if (material.alphaMask != 1.0f) {
                    opaqueDrawIndirectCommands.emplace_back(drawIndirectCommand);
                    opaqueDrawData.push_back(DrawData{.modelMatrixIndex = node->modelMatrixIndex,
                                                      .materialIndex = static_cast<uint32_t>(mesh.materialIndex),
                                                      .boundingSphere = mesh.boundingSphere});
                }
                if (material.alphaMask != 0.0f) {
                    transparentDrawIndirectCommands.emplace_back(drawIndirectCommand);
                    transparentDrawData.push_back(DrawData{.modelMatrixIndex = node->modelMatrixIndex,
                                                           .materialIndex = static_cast<uint32_t>(mesh.materialIndex),
                                                           .boundingSphere = mesh.boundingSphere});
                }
            }
        }
    }

    for (const auto &child: node->children) {
        DrawNode(child, frustumCulling);
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

void Scene::CreateBuffers() {

    materialsBuffer = std::make_shared<Buffer>(
            m_Device, BufferSpecification{.size = 128 * sizeof(Material), .type = BufferType::GPU});

    constexpr size_t maxLights = 128;
    lightsBuffer = std::make_shared<Buffer>(
            m_Device, BufferSpecification{.size = maxLights * sizeof(Light), .type = BufferType::GPU});

    constexpr size_t maxCameras = 8;
    camerasBuffer = std::make_shared<Buffer>(
            m_Device, BufferSpecification{.size = sizeof(Camera::CameraData) * maxCameras, .type = BufferType::GPU});

    modelMatricesBuffer = std::make_shared<Buffer>(
            m_Device,
            BufferSpecification{.size = globalModelMatrices.size() * sizeof(glm::mat4), .type = BufferType::GPU});

    constexpr size_t maxDrawIndirectCommands = 8192;
    opaqueDrawIndirectCommandsBuffer = std::make_shared<Buffer>(
            m_Device, BufferSpecification{.size = maxDrawIndirectCommands * sizeof(VkDrawIndexedIndirectCommand),
                                          .type = BufferType::GPU_INDIRECT});

    transparentDrawIndirectCommandsBuffer = std::make_shared<Buffer>(
            m_Device, BufferSpecification{.size = maxDrawIndirectCommands * sizeof(VkDrawIndexedIndirectCommand),
                                          .type = BufferType::GPU_INDIRECT});

    opaqueDrawDataBuffer = std::make_shared<Buffer>(
            m_Device, BufferSpecification{.size = maxDrawIndirectCommands * sizeof(DrawData), .type = BufferType::GPU});

    transparentDrawDataBuffer = std::make_shared<Buffer>(
            m_Device, BufferSpecification{.size = maxDrawIndirectCommands * sizeof(DrawData), .type = BufferType::GPU});

    constexpr size_t maxMeshes = 16384;
    meshesBuffer = std::make_shared<Buffer>(
            m_Device, BufferSpecification{.size = maxMeshes * sizeof(DrawData), .type = BufferType::GPU});
}
