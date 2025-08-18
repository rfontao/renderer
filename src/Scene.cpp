#include "Scene.h"
#include "pch.h"

#include <ranges>
#include <utility>

#include "Application.h"
#include "Vulkan/Buffer.h"


Scene::Scene(std::shared_ptr<VulkanDevice> device, const std::filesystem::path &scenePath,
             std::shared_ptr<TextureCube> skyboxTexture, DebugDraw &debugDraw) :
    skyboxTexture(std::move(skyboxTexture)), device(std::move(device)) {

    cameras.resize(2);
    cameras[0] = Camera(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                        (double) 1280 / (double) 720);

    // NOTE: Debug camera
    cameras[1] = Camera(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                        (double) 1280 / (double) 720);

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

    resourcePath = scenePath.parent_path();

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

    GenerateDrawCommands(debugDraw);
    CreateBuffers();
}

void Scene::CreateVertexBuffer(std::vector<Vertex> &vertices) {
    const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    const auto stagingBuffer = std::make_unique<Buffer>(
            device, BufferSpecification{
                            .name = "Vertex Buffer Staging Buffer", .size = bufferSize, .type = BufferType::STAGING});
    stagingBuffer->From(vertices.data(), bufferSize);

    vertexBuffer = std::make_unique<Buffer>(
            device, BufferSpecification{.name = "Vertex Buffer", .size = bufferSize, .type = BufferType::VERTEX});
    vertexBuffer->FromBuffer(stagingBuffer.get());
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
            device, BufferSpecification{
                            .name = "Skybox Staging Buffer", .size = skyboxBufferSize, .type = BufferType::STAGING});
    skyboxStagingBuffer->From(skyboxVertices.data(), skyboxBufferSize);

    skyboxVertexBuffer = std::make_unique<Buffer>(
            device,
            BufferSpecification{.name = "Skybox Vertex Buffer", .size = skyboxBufferSize, .type = BufferType::VERTEX});
    skyboxVertexBuffer->FromBuffer(skyboxStagingBuffer.get());
    skyboxStagingBuffer->Destroy();
}

void Scene::CreateIndexBuffer(std::vector<uint32_t> &indices) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    const auto stagingBuffer = std::make_unique<Buffer>(
            device, BufferSpecification{
                            .name = "Index Buffer Staging Buffer", .size = bufferSize, .type = BufferType::STAGING});
    stagingBuffer->From(indices.data(), bufferSize);

    indexBuffer = std::make_unique<Buffer>(
            device, BufferSpecification{.name = "Index Buffer", .size = bufferSize, .type = BufferType::INDEX});
    indexBuffer->FromBuffer(stagingBuffer.get());
    stagingBuffer->Destroy();
}

void Scene::LoadImages(tinygltf::Model &input) {
    images.resize(input.images.size() + 1);
    for (size_t i = 0; i < input.images.size(); i++) {
        tinygltf::Image &glTFImage = input.images[i];

        // Load from disk
        if (!glTFImage.uri.empty()) {
            TextureSpecification spec{.name = glTFImage.uri,
                                      .width = static_cast<uint32_t>(glTFImage.width),
                                      .height = static_cast<uint32_t>(glTFImage.height),
                                      .generateMipMaps = true};
            images[i] = std::make_shared<Texture2D>(device, spec, resourcePath / glTFImage.uri);
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
            TextureSpecification spec{.name = "Image loaded from buffer",
                                      .width = (uint32_t) glTFImage.width,
                                      .height = (uint32_t) glTFImage.height};
            images[i] = std::make_shared<Texture2D>(device, spec, buffer);
            if (deleteBuffer) {
                delete[] buffer;
            }
        }
    }

    // Default image/texture
    std::array<unsigned char, 1 * 1 * 4> pixels = {128, 128, 128, 255};
    TextureSpecification spec{.name = "Default Texture", .width = 1, .height = 1};
    images[images.size() - 1] = std::make_shared<Texture2D>(device, spec, pixels.data());
}

void Scene::LoadTextures(tinygltf::Model &input) {
    textures.resize(input.textures.size());
    for (size_t i = 0; i < input.textures.size(); i++) {
        textures[i].imageIndex = input.textures[i].source;
        if (input.textures[i].sampler != -1) {
            TextureSampler sampler = textureSamplers[input.textures[i].sampler];
            images[textures[i].imageIndex]->SetSampler(sampler);
        }
    }

    // TODO: Check better way to do later
    defaultTexture.imageIndex = static_cast<int32_t>(images.size() - 1);
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
        textureSamplers.push_back(sampler);
    }
}

void Scene::LoadMaterials(tinygltf::Model &input) {
    defaultMaterial = {};

    materials.resize(input.materials.size());
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

        materials[i] = material;
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

            AABB aabb = {};

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

                    aabb.min = glm::vec3{accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]};
                    aabb.max = glm::vec3{accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]};
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
            mesh.boundingBox = aabb;

            meshes.push_back(mesh);
            node->meshIndices.push_back(meshes.size() - 1);
        }
    }

    if (parent) {
        parent->children.push_back(node);
    } else {
        nodes.push_back(node);
    }
}

void Scene::Destroy() {
    indexBuffer->Destroy();
    vertexBuffer->Destroy();

    materialsBuffer->Destroy();
    lightsBuffer->Destroy();
    camerasBuffer->Destroy();

    for (const auto &node: nodes) {
        delete node;
    }

    //    m_DefaultImage.texture.Destroy();
    for (auto &image: images) {
        image->Destroy();
    }
}

void Scene::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const {
    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {vertexBuffer->GetBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

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
    } pushConstants{materialsBuffer->GetAddress(),        lightsBuffer->GetAddress(),
                    camerasBuffer->GetAddress(),          opaqueDrawDataBuffer->GetAddress(),
                    modelMatricesBuffer->GetAddress(),    0,
                    static_cast<uint32_t>(lights.size()), 800,
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
    VkBuffer vertexBuffers[] = {vertexBuffer->GetBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    struct shadowPushConstants {
        VkDeviceAddress lightBufferAddress;
        VkDeviceAddress drawDataBufferAddress;
        VkDeviceAddress modelMatricesBufferAddress;
        int32_t directionalLightIndex;
    } pushConstants{lightsBuffer->GetAddress(), opaqueDrawDataBuffer->GetAddress(), modelMatricesBuffer->GetAddress(),
                    0};
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

void Scene::DrawSkybox(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {

    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {skyboxVertexBuffer->GetBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    const struct SkyboxPushConstant {
        VkDeviceAddress cameraBufferAddress;
        uint32_t cameraIndex;
        uint32_t skyboxTextureIndex;
    } pushConstants{camerasBuffer->GetAddress(), Application::cameraIndexDrawing, 750};
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(SkyboxPushConstant), &pushConstants);
    vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void Scene::GenerateDrawCommands(DebugDraw &debugDraw, bool frustumCulling) {
    globalModelMatrices.resize(localModelMatrices.size());
    opaqueDrawData.clear();
    transparentDrawData.clear();
    opaqueDrawIndirectCommands.clear();
    transparentDrawIndirectCommands.clear();

    // Render all nodes at top-level
    for (const auto &node: nodes) {
        DrawNode(node, debugDraw, frustumCulling);
    }
}
void Scene::UploadToGPU(GPUDataUploader &uploader) {
    uploader.AddCopy(materials, materialsBuffer->GetBuffer());
    uploader.AddCopy(lights, lightsBuffer->GetBuffer());

    auto camerasGPUData = std::ranges::to<std::vector>(
            cameras | std::views::transform([](const Camera &camera) { return camera.GetGPUData(); }));
    uploader.AddCopy(camerasGPUData, camerasBuffer->GetBuffer());

    uploader.AddCopy(opaqueDrawIndirectCommands, opaqueDrawIndirectCommandsBuffer->GetBuffer());
    uploader.AddCopy(opaqueDrawData, opaqueDrawDataBuffer->GetBuffer());

    uploader.AddCopy(transparentDrawIndirectCommands, transparentDrawIndirectCommandsBuffer->GetBuffer());
    uploader.AddCopy(transparentDrawData, transparentDrawDataBuffer->GetBuffer());

    uploader.AddCopy(globalModelMatrices, modelMatricesBuffer->GetBuffer());
}

void Scene::DrawNode(Node *node, DebugDraw &debugDraw, bool frustumCulling) {
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

                const std::array corners = {
                        mesh.boundingBox.min,
                        glm::vec3(mesh.boundingBox.max.x, mesh.boundingBox.min.y, mesh.boundingBox.min.z),
                        glm::vec3(mesh.boundingBox.min.x, mesh.boundingBox.max.y, mesh.boundingBox.min.z),
                        glm::vec3(mesh.boundingBox.min.x, mesh.boundingBox.min.y, mesh.boundingBox.max.z),
                        glm::vec3(mesh.boundingBox.max.x, mesh.boundingBox.max.y, mesh.boundingBox.min.z),
                        glm::vec3(mesh.boundingBox.max.x, mesh.boundingBox.min.y, mesh.boundingBox.max.z),
                        glm::vec3(mesh.boundingBox.min.x, mesh.boundingBox.max.y, mesh.boundingBox.max.z),
                        mesh.boundingBox.max};

                glm::vec3 newMin(std::numeric_limits<float>::max());
                glm::vec3 newMax(std::numeric_limits<float>::lowest());
                for (size_t i = 0; i < 8; ++i) {
                    glm::vec4 transformed = nodeMatrix * glm::vec4(corners[i], 1.0f);
                    auto p = glm::vec3(transformed);
                    newMin = glm::min(newMin, p);
                    newMax = glm::max(newMax, p);
                }
                AABB aabb = {.min = newMin, .max = newMax};
                if (frustumCulling) {
                    if (camera.IsAABBFullyOutsideFrustum(aabb)) {
                        // debugDraw.DrawAABB(aabb, {1.0f, 0.0f, 0.0f});
                        continue;
                    }
                    // debugDraw.DrawAABB(aabb, {0.0f, 1.0f, 0.0f});
                }

                // Get the texture index for this primitive
                const auto &material = mesh.materialIndex != -1 ? materials[mesh.materialIndex] : defaultMaterial;

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
                                                      .boundingBox = aabb});
                }
                if (material.alphaMask != 0.0f) {
                    transparentDrawIndirectCommands.emplace_back(drawIndirectCommand);
                    transparentDrawData.push_back(DrawData{.modelMatrixIndex = node->modelMatrixIndex,
                                                           .materialIndex = static_cast<uint32_t>(mesh.materialIndex),
                                                           .boundingBox = aabb});
                }
            }
        }
    }

    for (const auto &child: node->children) {
        DrawNode(child, debugDraw, frustumCulling);
    }
}

void Scene::CreateLights() {
    constexpr auto lightDirection = glm::vec3(0.1f, 1.0f, 0.25f);
    lights.push_back(
            {.direction = lightDirection,
             .view = glm::lookAt(lightDirection * 15.0f, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
             .proj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 100.0f),
             .type = Light::Type::DIRECTIONAL});

    lights.push_back({.position = glm::vec3(-11.0f, 0.1f, -0.3f), .type = Light::Type::POINT});
    lights.push_back({.position = glm::vec3(-6.5f, 1.0f, -1.5f), .type = Light::Type::POINT});
}

void Scene::CreateBuffers() {

    materialsBuffer = std::make_unique<Buffer>(
            device,
            BufferSpecification{.name = "Materials Buffer", .size = 128 * sizeof(Material), .type = BufferType::GPU});

    constexpr size_t maxLights = 128;
    lightsBuffer = std::make_unique<Buffer>(
            device,
            BufferSpecification{.name = "Lights Buffer", .size = maxLights * sizeof(Light), .type = BufferType::GPU});

    constexpr size_t maxCameras = 8;
    camerasBuffer = std::make_unique<Buffer>(device, BufferSpecification{.name = "Cameras Buffer",
                                                                         .size = sizeof(Camera::GPUData) * maxCameras,
                                                                         .type = BufferType::GPU});

    modelMatricesBuffer =
            std::make_unique<Buffer>(device, BufferSpecification{.name = "Model Matrices Buffer",
                                                                 .size = globalModelMatrices.size() * sizeof(glm::mat4),
                                                                 .type = BufferType::GPU});

    constexpr size_t maxDrawIndirectCommands = 8192;
    opaqueDrawIndirectCommandsBuffer = std::make_unique<Buffer>(
            device, BufferSpecification{.name = "Opaque Draw Indirect Commands Buffer",
                                        .size = maxDrawIndirectCommands * sizeof(VkDrawIndexedIndirectCommand),
                                        .type = BufferType::GPU_INDIRECT});

    transparentDrawIndirectCommandsBuffer = std::make_unique<Buffer>(
            device, BufferSpecification{.name = "Transparent Draw Indirect Commands Buffer",
                                        .size = maxDrawIndirectCommands * sizeof(VkDrawIndexedIndirectCommand),
                                        .type = BufferType::GPU_INDIRECT});

    opaqueDrawDataBuffer =
            std::make_unique<Buffer>(device, BufferSpecification{.name = "Opaque Draw Data Buffer",
                                                                 .size = maxDrawIndirectCommands * sizeof(DrawData),
                                                                 .type = BufferType::GPU});

    transparentDrawDataBuffer =
            std::make_unique<Buffer>(device, BufferSpecification{.name = "Transparent Draw Data Buffer",
                                                                 .size = maxDrawIndirectCommands * sizeof(DrawData),
                                                                 .type = BufferType::GPU});

    constexpr size_t maxMeshes = 16384;
    meshesBuffer = std::make_unique<Buffer>(
            device, BufferSpecification{.name = "Meshes Buffer", .size = maxMeshes * sizeof(DrawData), .type = BufferType::GPU});
}
