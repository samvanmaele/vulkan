#include "vk_loadGLTF.hpp"
#include "vk_buffers.hpp"
#include "common.hpp"
#include <SDL3_image/SDL_image.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <numeric>
#include <stdexcept>
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
        BufferManager::stageBuffer(physicalDevice, device, graphicsQueue, commandPool, posAttrib.dataPtr, vertexCount * posAttrib.stride, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, primitiveData.posBuffer, primitiveData.positionBufferMemory);

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
                const tinygltf::Image& image = model.images[texIndex];
                createTextureImage(physicalDevice, device, graphicsQueue, commandPool, primitiveData.textureImage, primitiveData.textureImageMemory, image.image.data(), image.width, image.height);
                createTextureImageView(device, primitiveData.textureImageView, primitiveData.textureImage);
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

void VkModel::createTextureImage(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage &textureImage, VkDeviceMemory &textureImageMemory, const unsigned char* pixels, int texWidth, int texHeight)
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    int channels = 4;
    VkDeviceSize imageSize = texWidth * texHeight * channels;

    BufferManager::createBuffer(physicalDevice, device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    createImage(physicalDevice, device, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
    transitionImageLayout(device, graphicsQueue, commandPool, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    BufferManager::copyBufferToImage(device, graphicsQueue, commandPool, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(device, graphicsQueue, commandPool, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
void VkModel::createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    vk_check(vkCreateImage(device, &imageInfo, nullptr, &image), "failed to create image!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = BufferManager::findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    vk_check(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory), "failed to allocate image memory!");

    vkBindImageMemory(device, image, imageMemory, 0);
}
void VkModel::transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = BufferManager::beginSingleTimeCommands(device, commandPool);

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    BufferManager::endSingleTimeCommands(device, commandBuffer, graphicsQueue, commandPool);
}
void VkModel::createTextureImageView(VkDevice device, VkImageView &textureImageView, VkImage textureImage)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vk_check(vkCreateImageView(device, &viewInfo, nullptr, &textureImageView), "failed to create texture image view!");
}