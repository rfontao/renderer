#include "pch.h"
#include "Scene.h"

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
        throw std::runtime_error(
                "Could not open " + scenePath.string() + " scene file. Exiting... \n" + warning + error);
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

    VulkanBuffer stagingBuffer(m_Device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                      VMA_ALLOCATION_CREATE_MAPPED_BIT);

//    stagingBuffer.Map();
    stagingBuffer.From(vertices.data(), (size_t) bufferSize);
//    stagingBuffer.Unmap();

    m_VertexBuffer = VulkanBuffer(m_Device, bufferSize,
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                  VMA_MEMORY_USAGE_AUTO);
    m_VertexBuffer.FromBuffer(stagingBuffer);
    stagingBuffer.Destroy();
}

void Scene::CreateIndexBuffer(std::vector<uint32_t> &indices) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VulkanBuffer stagingBuffer(m_Device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
                               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                               VMA_ALLOCATION_CREATE_MAPPED_BIT);
//    stagingBuffer.Map();
    stagingBuffer.From(indices.data(), (size_t) bufferSize);
//    stagingBuffer.Unmap();

    m_IndexBuffer = VulkanBuffer(m_Device, bufferSize,
                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                 VMA_MEMORY_USAGE_AUTO);
    m_IndexBuffer.FromBuffer(stagingBuffer);
    stagingBuffer.Destroy();
}

void Scene::LoadImages(tinygltf::Model &input) {
    m_Images.resize(input.images.size());
    for (size_t i = 0; i < input.images.size(); i++) {
        tinygltf::Image &glTFImage = input.images[i];

        // Load from disk
        if (!glTFImage.uri.empty()) {
            m_Images[i].texture = VulkanTexture(m_Device, m_ResourcePath / glTFImage.uri);
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

            m_Images[i].texture = VulkanTexture(m_Device, buffer, bufferSize, glTFImage.width, glTFImage.height);
            if (deleteBuffer) {
                delete[] buffer;
            }
        }
    }

    m_DefaultImage.texture = VulkanTexture(m_Device);
}

void Scene::LoadTextures(tinygltf::Model &input) {
    m_Textures.resize(input.textures.size());
    for (size_t i = 0; i < input.textures.size(); i++) {
        m_Textures[i].imageIndex = input.textures[i].source + 1;
        if (input.textures[i].sampler != -1) {
            TextureSampler sampler = m_TextureSamplers[input.textures[i].sampler];
            m_Images[m_Textures[i].imageIndex - 1].texture.SetSampler(sampler.magFilter, sampler.minFilter,
                                                                  sampler.addressModeU, sampler.addressModeV,
                                                                  sampler.addressModeW);
        }
    }

    m_DefaultTexture.imageIndex = -1;
}

static VkSamplerAddressMode GetVkWrapMode(int32_t wrapMode) {
    switch (wrapMode) {
        case -1:
        case 10497:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case 33071:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case 33648:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        default:
            std::cerr << "Unknown wrap mode for getVkWrapMode: " << wrapMode << std::endl;
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

static VkFilter GetVkFilterMode(int32_t filterMode) {
    switch (filterMode) {
        case -1:
        case 9728:
        case 9984:
        case 9985:
            return VK_FILTER_NEAREST;
        case 9986:
        case 9987:
        case 9729:
            return VK_FILTER_LINEAR;
        default:
            std::cerr << "Unknown filter mode for getVkFilterMode: " << filterMode << std::endl;
            return VK_FILTER_NEAREST;
    }
}

void Scene::LoadTextureSamplers(tinygltf::Model &input) {
    for (const tinygltf::Sampler &smpl: input.samplers) {
        TextureSampler sampler{};
        sampler.minFilter = GetVkFilterMode(smpl.minFilter);
        sampler.magFilter = GetVkFilterMode(smpl.magFilter);
        sampler.addressModeU = GetVkWrapMode(smpl.wrapS);
        sampler.addressModeV = GetVkWrapMode(smpl.wrapT);
        sampler.addressModeW = sampler.addressModeV;
        m_TextureSamplers.push_back(sampler);
    }
}

void Scene::LoadMaterials(tinygltf::Model &input) {
    m_Materials.resize(input.materials.size());
    for (size_t i = 0; i < input.materials.size(); i++) {
        MaterialUBO material{}; // Must have a different name than below, or it doesn't work

        m_Materials[i].info = VulkanBuffer(m_Device, sizeof(MaterialUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                           VMA_MEMORY_USAGE_CPU_TO_GPU);
        m_Materials[i].info.Map();

        tinygltf::Material glTFMaterial = input.materials[i];
        if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
            material.baseColorFactor = glm::make_vec4(
                    glTFMaterial.values["baseColorFactor"].ColorFactor().data());
        }

        if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
            material.baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
            material.baseColorTextureUV = glTFMaterial.values["baseColorTexture"].TextureTexCoord();
        }

        if (glTFMaterial.values.find("roughnessFactor") != glTFMaterial.values.end()) {
            material.roughnessFactor = glm::vec4(static_cast<float>(glTFMaterial.values["roughnessFactor"].Factor()));
        }

        if (glTFMaterial.values.find("metallicFactor") != glTFMaterial.values.end()) {
            material.metallicFactor = glm::vec4(static_cast<float>(glTFMaterial.values["metallicFactor"].Factor()));
        }

        if (glTFMaterial.additionalValues.find("emissiveFactor") != glTFMaterial.additionalValues.end()) {
            material.emissiveFactor = glm::vec4(
                    glm::make_vec3(glTFMaterial.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
        }

        if (glTFMaterial.values.find("metallicRoughnessTexture") != glTFMaterial.values.end()) {
            material.metallicRoughnessTextureIndex = glTFMaterial.values["metallicRoughnessTexture"].TextureIndex();
            material.metallicRoughnessTextureUV = glTFMaterial.values["metallicRoughnessTexture"].TextureTexCoord();
        }

        // Get the normal map texture index
        if (glTFMaterial.additionalValues.find("normalTexture") != glTFMaterial.additionalValues.end()) {
            material.normalTextureIndex = glTFMaterial.additionalValues["normalTexture"].TextureIndex();
            material.normalTextureUV = glTFMaterial.additionalValues["normalTexture"].TextureTexCoord();
        }

        if (glTFMaterial.additionalValues.find("emissiveTexture") != glTFMaterial.additionalValues.end()) {
            material.emissiveTextureIndex = glTFMaterial.additionalValues["emissiveTexture"].TextureIndex();
            material.emissiveTextureUV = glTFMaterial.additionalValues["emissiveTexture"].TextureTexCoord();
        }

        if (glTFMaterial.additionalValues.find("alphaMode") != glTFMaterial.additionalValues.end()) {
            tinygltf::Parameter param = glTFMaterial.additionalValues["alphaMode"];
            if (param.string_value == "BLEND") {
                material.alphaMask = 2.0f;
            }
            if (param.string_value == "MASK") {
                material.alphaCutoff = 0.5f;
                material.alphaMask = 1.0f;
            }
        }
        if (glTFMaterial.additionalValues.find("alphaCutoff") != glTFMaterial.additionalValues.end()) {
            material.alphaCutoff = static_cast<float>(glTFMaterial.additionalValues["alphaCutoff"].Factor());
        }

        m_Materials[i].info.From(&material, sizeof(material));
        m_Materials[i].ubo = material;

        std::vector<VkDescriptorSetLayoutBinding> materialSetLayout = {
//                {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
//                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
//                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
//                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        };

        // TODO: Create all pipelines in advance and store setlayouts on device??
        VkDescriptorSetLayoutCreateInfo layoutInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = static_cast<uint32_t>(materialSetLayout.size()),
                .pBindings = materialSetLayout.data(),
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

        VK_CHECK(vkAllocateDescriptorSets(m_Device->GetDevice(), &setCreateInfo, &m_Materials[i].descriptorSet),
                 "Failed to allocate descriptor sets");


//        auto &tex = material.baseColorTextureIndex != -1
//                    ? m_Images[m_Textures[material.baseColorTextureIndex].imageIndex].texture
//                    : m_DefaultImage.texture;
//        VkDescriptorImageInfo imageInfo{
//                .sampler = tex.GetSampler(),
//                .imageView = tex.GetImage()->GetImageView(),
//                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//        };
//
//
//        auto &tex1 = material.normalTextureIndex != -1
//                     ? m_Images[m_Textures[material.normalTextureIndex].imageIndex].texture
//                     : m_DefaultImage.texture;
//        VkDescriptorImageInfo normalImageInfo{
//                .sampler = tex1.GetSampler(),
//                .imageView = tex1.GetImage()->GetImageView(),
//                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//        };
//
//        auto &tex2 = material.metallicRoughnessTextureIndex != -1
//                     ? m_Images[m_Textures[material.metallicRoughnessTextureIndex].imageIndex].texture
//                     : m_DefaultImage.texture;
//        VkDescriptorImageInfo metallicRoughnessImageInfo{
//                .sampler = tex2.GetSampler(),
//                .imageView = tex2.GetImage()->GetImageView(),
//                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//        };
//
//        auto &tex3 = material.emissiveTextureIndex != -1
//                     ? m_Images[m_Textures[material.emissiveTextureIndex].imageIndex].texture
//                     : m_DefaultImage.texture;
//        VkDescriptorImageInfo emissiveImageInfo{
//                .sampler = tex3.GetSampler(),
//                .imageView = tex3.GetImage()->GetImageView(),
//                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//        };


        VkDescriptorBufferInfo bufferInfo{
                .buffer = m_Materials[i].info.GetBuffer(),
                .offset = 0,
                .range = sizeof(MaterialUBO),
        };

//        std::array<VkWriteDescriptorSet, 5> writeDescriptorSets{};
//        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        writeDescriptorSets[0].dstSet = m_Materials[i].descriptorSet;
//        writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        writeDescriptorSets[0].dstBinding = 0;
//        writeDescriptorSets[0].pImageInfo = &imageInfo;
//        writeDescriptorSets[0].descriptorCount = 1;
//
//        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        writeDescriptorSets[1].dstSet = m_Materials[i].descriptorSet;
//        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        writeDescriptorSets[1].dstBinding = 1;
//        writeDescriptorSets[1].pImageInfo = &normalImageInfo;
//        writeDescriptorSets[1].descriptorCount = 1;
//
//        writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        writeDescriptorSets[2].dstSet = m_Materials[i].descriptorSet;
//        writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        writeDescriptorSets[2].dstBinding = 2;
//        writeDescriptorSets[2].pImageInfo = &metallicRoughnessImageInfo;
//        writeDescriptorSets[2].descriptorCount = 1;
//
//        writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        writeDescriptorSets[3].dstSet = m_Materials[i].descriptorSet;
//        writeDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        writeDescriptorSets[3].dstBinding = 3;
//        writeDescriptorSets[3].pImageInfo = &emissiveImageInfo;
//        writeDescriptorSets[3].descriptorCount = 1;


        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = m_Materials[i].descriptorSet;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.pBufferInfo = &bufferInfo;
        writeDescriptorSet.descriptorCount = 1;

        vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &writeDescriptorSet, 0, nullptr);

        vkDestroyDescriptorSetLayout(m_Device->GetDevice(), layout, nullptr);
    }

    m_DefaultMaterial.info = VulkanBuffer(m_Device, sizeof(MaterialUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                          VMA_MEMORY_USAGE_CPU_TO_GPU);
    m_DefaultMaterial.info.Map();

    MaterialUBO mat{};
    m_DefaultMaterial.info.From(&mat, sizeof(mat));
    m_DefaultMaterial.ubo = mat;

    std::vector<VkDescriptorSetLayoutBinding> materialSetLayout = {
//            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
//            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
//            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    };

    // TODO: Create all pipelines in advance and store setlayouts on device??
    VkDescriptorSetLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(materialSetLayout.size()),
            .pBindings = materialSetLayout.data(),
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

    VK_CHECK(vkAllocateDescriptorSets(m_Device->GetDevice(), &setCreateInfo, &m_DefaultMaterial.descriptorSet),
             "Failed to allocate descriptor sets");


//    VkDescriptorImageInfo imageInfo{
//            .sampler = m_DefaultImage.texture.GetSampler(),
//            .imageView = m_DefaultImage.texture.GetImage()->GetImageView(),
//            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//    };
//
//    VkDescriptorImageInfo normalImageInfo{
//            .sampler = m_DefaultImage.texture.GetSampler(),
//            .imageView = m_DefaultImage.texture.GetImage()->GetImageView(),
//            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//    };
//
//    VkDescriptorImageInfo metallicRoughnessImageInfo{
//            .sampler = m_DefaultImage.texture.GetSampler(),
//            .imageView = m_DefaultImage.texture.GetImage()->GetImageView(),
//            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//    };

    VkDescriptorBufferInfo bufferInfo{
            .buffer = m_DefaultMaterial.info.GetBuffer(),
            .offset = 0,
            .range = sizeof(MaterialUBO),
    };

//    std::array<VkWriteDescriptorSet, 4> writeDescriptorSets{};
//    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    writeDescriptorSets[0].dstSet = m_DefaultMaterial.descriptorSet;
//    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    writeDescriptorSets[0].dstBinding = 0;
//    writeDescriptorSets[0].pImageInfo = &imageInfo;
//    writeDescriptorSets[0].descriptorCount = 1;
//
//    writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    writeDescriptorSets[1].dstSet = m_DefaultMaterial.descriptorSet;
//    writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    writeDescriptorSets[1].dstBinding = 1;
//    writeDescriptorSets[1].pImageInfo = &normalImageInfo;
//    writeDescriptorSets[1].descriptorCount = 1;
//
//    writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    writeDescriptorSets[2].dstSet = m_DefaultMaterial.descriptorSet;
//    writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    writeDescriptorSets[2].dstBinding = 2;
//    writeDescriptorSets[2].pImageInfo = &metallicRoughnessImageInfo;
//    writeDescriptorSets[2].descriptorCount = 1;

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = m_DefaultMaterial.descriptorSet;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.pBufferInfo = &bufferInfo;
    writeDescriptorSet.descriptorCount = 1;

    vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &writeDescriptorSet, 0, nullptr);

    vkDestroyDescriptorSetLayout(m_Device->GetDevice(), layout, nullptr);
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
                    texCoordsBuffer0 = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[
                            accessor.byteOffset + view.byteOffset]));
                }

                if (glTFPrimitive.attributes.find("TEXCOORD_1") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor &accessor = input.accessors[glTFPrimitive.attributes.find(
                            "TEXCOORD_1")->second];
                    const tinygltf::BufferView &view = input.bufferViews[accessor.bufferView];
                    texCoordsBuffer1 = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[
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

    m_SceneInfo.Destroy();

    for (auto node: m_Nodes) {
        delete node;
    }

    m_DefaultMaterial.info.Destroy();
    for (auto &material: m_Materials) {
        material.info.Destroy();
    }

    m_DefaultImage.texture.Destroy();
    for (auto &image: m_Images) {
        image.texture.Destroy();
    }
}

void Scene::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, bool isSkybox) {
    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {m_VertexBuffer.GetBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    if (!isSkybox)
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1,
                                &m_SceneInfoDescriptorSet, 0, nullptr);
    // Render all nodes at top-level
    for (auto &node: m_Nodes) {
        DrawNode(commandBuffer, pipelineLayout, node, OPAQUE, isSkybox);
    }

    for (auto &node: m_Nodes) {
        DrawNode(commandBuffer, pipelineLayout, node, MASK, isSkybox);
    }
}

void
Scene::DrawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Node *node, AlphaMode alphaMode,
                bool isSkybox) {
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
        // Pass the final matrix to the vertex shader using push constants
        if (!isSkybox)
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                               &nodeMatrix);
        for (Primitive &primitive: node->mesh.primitives) {
            if (primitive.indexCount > 0) {
                // Get the texture index for this primitive
                auto &material =
                        primitive.materialIndex != -1 ? m_Materials[primitive.materialIndex] : m_DefaultMaterial;

                if (alphaMode == OPAQUE && material.ubo.alphaMask != 0.0f)
                    continue;

                if (alphaMode == MASK && material.ubo.alphaMask != 1.0f)
                    continue;

                if (!isSkybox)
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 3, 1,
                                            &material.descriptorSet, 0, nullptr);

                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
            }
        }
    }
    for (auto &child: node->children) {
        DrawNode(commandBuffer, pipelineLayout, child, alphaMode, isSkybox);
    }

}

void Scene::CreateLights() {

    SceneInfo sceneInfo{};
    sceneInfo.lightDir = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneInfo.lightCount = 0;
    sceneInfo.lightPos[0] = glm::vec3(-11.0f, 0.1f, -0.3f);
    sceneInfo.lightPos[1] = glm::vec3(-6.5f, 1.0f, -1.5f);

    VkDescriptorSetLayoutBinding sceneInfoSetLayout = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                                                       VK_SHADER_STAGE_FRAGMENT_BIT,
                                                       nullptr};


    VkDescriptorSetLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &sceneInfoSetLayout,
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

    VK_CHECK(vkAllocateDescriptorSets(m_Device->GetDevice(), &setCreateInfo, &m_SceneInfoDescriptorSet),
             "Failed to allocate descriptor sets");


    m_SceneInfo = VulkanBuffer(m_Device, sizeof(SceneInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VMA_MEMORY_USAGE_CPU_ONLY);
    m_SceneInfo.Map();
    m_SceneInfo.From(&sceneInfo, sizeof(sceneInfo));

    VkDescriptorBufferInfo bufferInfo{
            .buffer = m_SceneInfo.GetBuffer(),
            .offset = 0,
            .range = sizeof(SceneInfo),
    };

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = m_SceneInfoDescriptorSet;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.pBufferInfo = &bufferInfo;
    writeDescriptorSet.descriptorCount = 1;

    vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &writeDescriptorSet, 0, nullptr);

    vkDestroyDescriptorSetLayout(m_Device->GetDevice(), layout, nullptr);
}

