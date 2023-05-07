#include "pch.h"
#include "Scene.h"

Scene::Scene(std::shared_ptr<VulkanDevice> device, const std::filesystem::path &filePath) : m_Device(device) {
    tinygltf::TinyGLTF gltfContext;
    tinygltf::Model glTFInput;
    std::string error, warning;
    bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filePath.string());
    if (!fileLoaded) {
        throw std::runtime_error("Could not open " + filePath.string() + " gltf file. Exiting...");
    }

    m_ResourcePath = filePath.parent_path();

    std::vector<uint32_t> indexBuffer;
    std::vector<Vertex> vertexBuffer;

    LoadImages(glTFInput);
    LoadMaterials(glTFInput);
    LoadTextures(glTFInput);
    const tinygltf::Scene &scene = glTFInput.scenes[0];
    for (int i: scene.nodes) {
        const tinygltf::Node node = glTFInput.nodes[i];
        LoadNode(glTFInput, node, nullptr, vertexBuffer, indexBuffer);
    }

    CreateVertexBuffer(vertexBuffer);
    CreateIndexBuffer(indexBuffer);
}

void Scene::CreateVertexBuffer(std::vector<Vertex> &vertices) {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VulkanBuffer stagingBuffer(m_Device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    stagingBuffer.Map();
    stagingBuffer.From(vertices.data(), (size_t) bufferSize);
    stagingBuffer.Unmap();

    m_VertexBuffer = VulkanBuffer(m_Device, bufferSize,
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_VertexBuffer.FromBuffer(stagingBuffer);
    stagingBuffer.Destroy();
}

void Scene::CreateIndexBuffer(std::vector<uint32_t> &indices) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VulkanBuffer stagingBuffer(m_Device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.Map();
    stagingBuffer.From(indices.data(), (size_t) bufferSize);
    stagingBuffer.Unmap();

    m_IndexBuffer = VulkanBuffer(m_Device, bufferSize,
                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_IndexBuffer.FromBuffer(stagingBuffer);
    stagingBuffer.Destroy();
}

void Scene::LoadImages(tinygltf::Model &input) {
    m_Images.resize(input.images.size());
    for (size_t i = 0; i < input.images.size(); i++) {
        tinygltf::Image &glTFImage = input.images[i];
        m_Images[i].texture = VulkanTexture(m_Device, m_ResourcePath / glTFImage.uri);


        VkDescriptorSetLayoutBinding samplerLayoutBinding{
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
        };

        // TODO: Create all pipelines in advance and store setlayouts on device??
        std::array<VkDescriptorSetLayoutBinding, 1> bindings = {samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 1,
                .pBindings = bindings.data(),
        };

        VkDescriptorSetLayout layout;
        VK_CHECK(vkCreateDescriptorSetLayout(m_Device->GetDevice(), &layoutInfo, nullptr, &layout),
                 "Failed to create descriptor set layout!");

        std::array<VkDescriptorSetLayout, 1> setLayouts = {layout};
        VkDescriptorSetAllocateInfo setCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = m_Device->GetDescriptorPool(),
                .descriptorSetCount = 1,
                .pSetLayouts = setLayouts.data(),
        };

        VK_CHECK(vkAllocateDescriptorSets(m_Device->GetDevice(), &setCreateInfo, &m_Images[i].descriptorSet),
                 "Failed to allocate descriptor sets");


        VkDescriptorImageInfo imageInfo{
                .sampler = m_Images[i].texture.GetSampler(),
                .imageView = m_Images[i].texture.GetImage()->GetImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = m_Images[i].descriptorSet;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.pImageInfo = &imageInfo;
        writeDescriptorSet.descriptorCount = 1;

        vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &writeDescriptorSet, 0, nullptr);
    }

    m_DefaultImage.texture = VulkanTexture(m_Device);


    VkDescriptorSetLayoutBinding samplerLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
    };

    // TODO: Create all pipelines in advance and store setlayouts on device??
    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = bindings.data(),
    };

    VkDescriptorSetLayout layout;
    VK_CHECK(vkCreateDescriptorSetLayout(m_Device->GetDevice(), &layoutInfo, nullptr, &layout),
             "Failed to create descriptor set layout!");

    std::array<VkDescriptorSetLayout, 1> setLayouts = {layout};
    VkDescriptorSetAllocateInfo setCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_Device->GetDescriptorPool(),
            .descriptorSetCount = 1,
            .pSetLayouts = setLayouts.data(),
    };

    VK_CHECK(vkAllocateDescriptorSets(m_Device->GetDevice(), &setCreateInfo, &m_DefaultImage.descriptorSet),
             "Failed to allocate descriptor sets");


    VkDescriptorImageInfo imageInfo{
            .sampler = m_DefaultImage.texture.GetSampler(),
            .imageView = m_DefaultImage.texture.GetImage()->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = m_DefaultImage.descriptorSet;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.pImageInfo = &imageInfo;
    writeDescriptorSet.descriptorCount = 1;

    vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &writeDescriptorSet, 0, nullptr);
}

void Scene::LoadTextures(tinygltf::Model &input) {
    m_Textures.resize(input.textures.size());
    for (size_t i = 0; i < input.textures.size(); i++) {
        m_Textures[i].imageIndex = input.textures[i].source;
    }

    m_DefaultTexture.imageIndex = -1;
}

void Scene::LoadMaterials(tinygltf::Model &input) {
    m_Materials.resize(input.materials.size());
    for (size_t i = 0; i < input.materials.size(); i++) {
        tinygltf::Material glTFMaterial = input.materials[i];
        if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
            m_Materials[i].baseColorFactor = glm::make_vec4(
                    glTFMaterial.values["baseColorFactor"].ColorFactor().data());
        }

        if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
            m_Materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
        } else {
            m_Materials[i].baseColorTextureIndex = -1;
        }
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
                const float *texCoordsBuffer = nullptr;
                const float *colorBuffer = nullptr;
                size_t vertexCount = 0;

                // Get buffer data for vertex positions
                if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.attributes.find(
                            "POSITION")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    positionBuffer = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[
                            accessor.byteOffset + view.byteOffset]));
                    vertexCount = accessor.count;
                }
                // Get buffer data for vertex normals
                if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.attributes.find(
                            "NORMAL")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    normalsBuffer = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[
                            accessor.byteOffset + view.byteOffset]));
                }
                // Get buffer data for vertex texture coordinates
                // glTF supports multiple sets, we only load the first one
                if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.attributes.find(
                            "TEXCOORD_0")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    texCoordsBuffer = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[
                            accessor.byteOffset + view.byteOffset]));
                }

                // Vertex colors
                if (glTFPrimitive.attributes.find("COLOR_0") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.attributes.find(
                            "COLOR_0")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    colorBuffer = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[
                            accessor.byteOffset + view.byteOffset]));

                }

                // Append data to model's vertex buffer
                for (size_t v = 0; v < vertexCount; v++) {
                    Vertex vert{};
                    vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                    vert.normal = glm::normalize(
                            glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                    vert.texCoord = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                    vert.color = colorBuffer ? glm::make_vec4(&colorBuffer[v * 3]) : glm::vec3(1.0f);
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
                        const auto *buf = reinterpret_cast<const uint32_t *>(&buffer.data[accessor.byteOffset +
                                                                                          bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        const auto *buf = reinterpret_cast<const uint16_t *>(&buffer.data[accessor.byteOffset +
                                                                                          bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        const auto *buf = reinterpret_cast<const uint8_t *>(&buffer.data[accessor.byteOffset +
                                                                                         bufferView.byteOffset]);
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

    m_DefaultImage.texture.Destroy();
    for (auto &image: m_Images) {
        image.texture.Destroy();
    }
}

void Scene::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {m_VertexBuffer.GetBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    // Render all nodes at top-level
    for (auto &node: m_Nodes) {
        DrawNode(commandBuffer, pipelineLayout, node);
    }
}

void Scene::DrawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Node *node) {
    if (!node->mesh.primitives.empty()) {
        // Pass the node's matrix via push constants
        // Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
        // TODO: Inefficient?
        glm::mat4 nodeMatrix = node->matrix;
        Node *currentParent = node->parent;
        while (currentParent) {
            nodeMatrix = currentParent->matrix * nodeMatrix;
            currentParent = currentParent->parent;
        }
        // Pass the final matrix to the vertex shader using push constants
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                           &nodeMatrix);
        for (Primitive &primitive: node->mesh.primitives) {
            if (primitive.indexCount > 0) {
                // Get the texture index for this primitive
                auto textureIndex = m_Materials[primitive.materialIndex].baseColorTextureIndex;
                // Bind the descriptor for the current primitive's texture
                if (textureIndex == -1) {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                                            &m_DefaultImage.descriptorSet, 0, nullptr);
                } else {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                                            &m_Images[m_Textures[textureIndex].imageIndex].descriptorSet, 0, nullptr);
                }
                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
            }
        }
    }
    for (auto &child: node->children) {
        DrawNode(commandBuffer, pipelineLayout, child);
    }

}
