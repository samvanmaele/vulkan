#include "vk_loadGLTF.hpp"
#include "common.hpp"
#include <SDL3_image/SDL_image.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vulkan/vulkan_core.h>

Model::Model(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, const char* filename)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!warn.empty()) {std::cout << "Warning: " << warn << std::endl;}
    if (!err.empty()) {std::cerr << "Error: " << err << std::endl;}
    if (!res) {std::cerr << "Failed to load glTF: " << filename << std::endl;}

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];

    for (int nodeIndex : scene.nodes)
    {
        const tinygltf::Node& node = model.nodes[nodeIndex];
        bindNode(physicalDevice, device, graphicsQueue, commandPool, model, node);
    }
}
void Model::bindNode(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, tinygltf::Model& model, const tinygltf::Node& node)
{
    if (node.mesh >= 0) bindMesh(physicalDevice, device, graphicsQueue, commandPool, model, model.meshes[node.mesh]);
    for (int child : node.children)
    {
        if (child >= 0) bindNode(physicalDevice, device, graphicsQueue, commandPool, model, model.nodes[child]);
    }
}
void Model::bindMesh(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, tinygltf::Model& model, tinygltf::Mesh& mesh)
{
    for (const auto& primitive : mesh.primitives)
    {
        PrimitiveData primitiveData;
        size_t vertexCount = 0;
        size_t indexCount = 0;

        AttribDatta posAttrib, normAttrib, texAttrib;
        for (const auto& attrib : primitive.attributes)
        {
            if (attrib.first == "POSITION") posAttrib = getAttrib(model, vertexCount, attrib.second, true);
            else if (attrib.first == "NORMAL") normAttrib = getAttrib(model, vertexCount, attrib.second, false);
            else if (attrib.first == "TEXCOORD_0") texAttrib = getAttrib(model, vertexCount, attrib.second, false);
        }
        primitiveData.vertices.resize(vertexCount);

        for (size_t i = 0; i < vertexCount; i++)
        {
            const float* pos = reinterpret_cast<const float*>(posAttrib.dataPtr + i * posAttrib.stride);
            const float* norm = reinterpret_cast<const float*>(normAttrib.dataPtr + i * normAttrib.stride);
            const float* tex = reinterpret_cast<const float*>(texAttrib.dataPtr + i * texAttrib.stride);
            primitiveData.vertices[i].pos = glm::vec3(pos[0], pos[1], pos[2]);
            primitiveData.vertices[i].normal = glm::vec3(norm[0], norm[1], norm[2]);
            primitiveData.vertices[i].texCoord = glm::vec2(tex[0], tex[1]);
        }

        if (primitive.indices >= 0)
        {
            AttribDatta indexAttrib = getAttrib(model, indexCount, primitive.indices, true);
            primitiveData.indices.resize(indexCount);
            for (size_t i = 0; i < indexCount; ++i)
            {
                uint32_t value = 0;
                memcpy(&value, indexAttrib.dataPtr + i * indexAttrib.stride, indexAttrib.stride);
                primitiveData.indices[i] = value;
            }
        }
        else
        {
            primitiveData.indices.resize(vertexCount);
            for (uint32_t i = 0; i < vertexCount; ++i)
            {
                primitiveData.indices[i] = i;
            }
        }

        createVertexBuffer(physicalDevice, device, graphicsQueue, commandPool, primitiveData.vertices, primitiveData.vertexBuffer, primitiveData.vertexBufferMemory);
        createIndexBuffer(physicalDevice, device, graphicsQueue, commandPool, primitiveData.indices, primitiveData.indexBuffer, primitiveData.indexBufferMemory);

        primitiveDataList.push_back(std::move(primitiveData));
    }
}
AttribDatta Model::getAttrib(tinygltf::Model& model, size_t &vecSize, int attribPos, bool calculatingPositions)
{
    const auto& accessor = model.accessors[attribPos];
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];

    if (calculatingPositions) vecSize = accessor.count;

    AttribDatta attrib
    {
        .dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset,
        .stride = accessor.ByteStride(bufferView),
    };

    return attrib;
}

void Model::createVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, std::vector<Vertex> vertices, VkBuffer &vertexBuffer, VkDeviceMemory &vertexBufferMemory)
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    copyBuffer(device, graphicsQueue, commandPool, stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
void Model::createIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, std::vector<uint32_t> indices, VkBuffer &indexBuffer, VkDeviceMemory &indexBufferMemory)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(device, graphicsQueue, commandPool, stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
void Model::createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk_check(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer), "failed to create buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    vk_check(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory), "failed to allocate buffer memory!");
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
uint32_t Model::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}
void Model::copyBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(device, commandBuffer, graphicsQueue, commandPool);
}
VkCommandBuffer Model::beginSingleTimeCommands(VkDevice &device, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}
void Model::endSingleTimeCommands(VkDevice &device, VkCommandBuffer &commandBuffer, VkQueue &graphicsQueue, VkCommandPool commandPool)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

/*
void Model::init(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue)
{
    Model("models/vedal987/vedal987.gltf");
    createTextureImage(physicalDevice, device, graphicsQueue, "models/vedal987/vedal987.png");
    createTextureImageView(device);
    createTextureSampler(physicalDevice, device);
    createVertexBuffer(physicalDevice, device, graphicsQueue);
    createIndexBuffer(physicalDevice, device, graphicsQueue);
}
void Model::createTextureImage(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkImage textureImage, VkDeviceMemory textureImageMemory, const char* filepath)
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    SDL_Surface *image = IMG_Load(filepath);
    const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(image->format);
    int channels = details->bytes_per_pixel;
    VkDeviceSize imageSize = image->h * image->w * channels;

    createBuffer(physicalDevice, device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, image->pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    createImage(physicalDevice, device, image->w, image->h, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
    transitionImageLayout(device, graphicsQueue, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(device, graphicsQueue, stagingBuffer, textureImage, static_cast<uint32_t>(image->w), static_cast<uint32_t>(image->h));
    transitionImageLayout(device, graphicsQueue, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    SDL_DestroySurface(image);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
void Model::createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
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
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    vk_check(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory), "failed to allocate image memory!");

    vkBindImageMemory(device, image, imageMemory, 0);
}
void Model::transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

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
    endSingleTimeCommands(device, commandBuffer, graphicsQueue);
}
void Model::copyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(device, commandBuffer, graphicsQueue);
}
void Model::createTextureImageView(VkDevice device, VkImage textureImage, VkDeviceMemory textureImageMemory)
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
void Model::createTextureSampler(VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    vk_check(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler), "failed to create texture sampler!");
}
*/
void Model::destroyAll(VkDevice device)
{
    for (PrimitiveData primitiveData : primitiveDataList)
    {
        //primitiveData.destroyTexture(device);
        primitiveData.destroyVertexBuffers(device);
    }
}