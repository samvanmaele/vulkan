#include "vk_loadGLTF.hpp"
#include "vk_buffers.hpp"
#include "common.hpp"
#include <SDL3_image/SDL_image.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <numeric>
#include <utility>

VkModel::VkModel(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, const char* filename)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!warn.empty()) std::cout << "Warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "Error: " << err << std::endl;
    if (!res) std::cerr << "Failed to load glTF: " << filename << std::endl;

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];

    for (int nodeIndex : scene.nodes)
    {
        const tinygltf::Node& node = model.nodes[nodeIndex];
        bindNode(physicalDevice, device, graphicsQueue, commandPool, model, node);
    }

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
    VkDeviceSize bufferSize = sizeof(ObjectUniformBufferObject);

    for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
    {
        BufferManager::createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[j], uniformBuffersMemory[j]);
        vkMapMemory(device, uniformBuffersMemory[j], 0, bufferSize, 0, &uniformBuffersMapped[j]);
    }
}
void VkModel::destroyAll(VkDevice device)
{
    for (PrimitiveData primitiveData : primitiveDataList)
    {
        primitiveData.destroyTexture(device);
        primitiveData.destroyVertexBuffers(device);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }
}

void VkModel::bindNode(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, tinygltf::Model& model, const tinygltf::Node& node)
{
    if (node.mesh >= 0) bindMesh(physicalDevice, device, graphicsQueue, commandPool, model, model.meshes[node.mesh]);
    for (int child : node.children)
    {
        if (child >= 0) bindNode(physicalDevice, device, graphicsQueue, commandPool, model, model.nodes[child]);
    }
}
void VkModel::bindMesh(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, tinygltf::Model& model, tinygltf::Mesh& mesh)
{
    for (const auto& primitive : mesh.primitives)
    {
        PrimitiveData primitiveData;
        size_t vertexCount, indexCount = 0;

        AttribDatta posAttrib, normAttrib, texAttrib;
        for (const auto& attrib : primitive.attributes)
        {
            if (attrib.first == "POSITION") posAttrib = getAttrib(model, vertexCount, attrib.second, true);
            else if (attrib.first == "NORMAL") normAttrib = getAttrib(model, vertexCount, attrib.second, false);
            else if (attrib.first == "TEXCOORD_0") texAttrib = getAttrib(model, vertexCount, attrib.second, false);
        }
        primitiveData.vertexCount = vertexCount;
        BufferManager::stageBuffer(physicalDevice, device, graphicsQueue, commandPool, posAttrib.dataPtr, vertexCount * posAttrib.stride, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, primitiveData.positionBuffer, primitiveData.positionBufferMemory);

        if (normAttrib.dataPtr)
        {
            BufferManager::stageBuffer(physicalDevice, device, graphicsQueue, commandPool, normAttrib.dataPtr, vertexCount * normAttrib.stride, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, primitiveData.normalBuffer, primitiveData.normalBufferMemory);
        }
        if (texAttrib.dataPtr)
        {
            BufferManager::stageBuffer(physicalDevice, device, graphicsQueue, commandPool, texAttrib.dataPtr, vertexCount * texAttrib.stride, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, primitiveData.texBuffer, primitiveData.texBufferMemory);
        }
        if (primitive.indices >= 0)
        {
            AttribDatta indexAttrib = getAttrib(model, indexCount, primitive.indices, true);
            primitiveData.indexCount = indexCount;
            primitiveData.indexType = indexAttrib.stride == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
            BufferManager::stageBuffer(physicalDevice, device, graphicsQueue, commandPool, indexAttrib.dataPtr, indexAttrib.stride * indexCount, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, primitiveData.indexBuffer, primitiveData.indexBufferMemory);
        }
        else
        {
            primitiveData.indexCount = vertexCount;
            std::vector<uint32_t> indices(vertexCount);
            std::iota(indices.begin(), indices.end(), 0);
            BufferManager::stageBuffer(physicalDevice, device, graphicsQueue, commandPool, indices.data(), indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, primitiveData.indexBuffer, primitiveData.indexBufferMemory);
        }

        if (primitive.material >= 0)
        {
            const auto& material = model.materials[primitive.material];
            if (material.values.find("baseColorTexture") != material.values.end())
            {
                int texIndex = material.values.at("baseColorTexture").TextureIndex();
                primitiveData.textureIndex = texIndex;
                createTexture(physicalDevice, device, graphicsQueue, commandPool, primitiveData, model.images[texIndex]);
            }
        }

        primitiveDataList.push_back(std::move(primitiveData));
    }
}
AttribDatta VkModel::getAttrib(tinygltf::Model& model, size_t &vecSize, int attribPos, bool calculatingPositions)
{
    const auto& accessor = model.accessors[attribPos];
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];

    if (calculatingPositions) vecSize = accessor.count;

    AttribDatta attrib
    {
        .dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset,
        .stride = accessor.ByteStride(bufferView)
    };

    return attrib;
}
void VkModel::createTexture(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, PrimitiveData &primitiveData, const tinygltf::Image& image)
{
    int channels = 4;
    VkDeviceSize imageSize = image.width * image.height * channels;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManager::createBuffer(physicalDevice, device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, image.image.data(), static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    BufferManager::createImage(physicalDevice, device, image.width, image.height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, primitiveData.textureImage, primitiveData.textureImageMemory, 0);
    BufferManager::transitionImageLayout(device, graphicsQueue, commandPool, primitiveData.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
    BufferManager::copyBufferToImage(device, graphicsQueue, commandPool, stagingBuffer, primitiveData.textureImage, image.width, image.height, 1);
    BufferManager::transitionImageLayout(device, graphicsQueue, commandPool, primitiveData.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
    BufferManager::createImageView(device, primitiveData.textureImage, primitiveData.textureImageView, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}