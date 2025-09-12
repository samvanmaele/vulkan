#pragma once
#include "common.hpp"

#include <tinygltf/tiny_gltf.h>

struct AttribDatta
{
    const unsigned char* dataPtr;
    int stride;
};

class Model
{
    public:
        std::vector<PrimitiveData> primitiveDataList;

        Model() = default;
        Model(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, const char* filename);

        /*
        void init(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue);
        void createTextureImage(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkImage textureImage, VkDeviceMemory textureImageMemory, const char* filepath);
        void createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        void transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void createTextureImageView(VkDevice device, VkImage textureImage, VkDeviceMemory textureImageMemory);
        void createTextureSampler(VkPhysicalDevice physicalDevice, VkDevice device);
        void createDescriptorSetLayout(VkDevice device);
        */

        void destroyAll(VkDevice device);

    private:
        /*
        void copyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        */
        void stageBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, const void* srcData, size_t dataSize, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
        void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void copyBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        VkCommandBuffer beginSingleTimeCommands(VkDevice &device, VkCommandPool commandPool);
        void endSingleTimeCommands(VkDevice &device, VkCommandBuffer &commandBuffer, VkQueue &graphicsQueue, VkCommandPool commandPool);

        void bindNode(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, tinygltf::Model& model, const tinygltf::Node& node);
        void bindMesh(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, tinygltf::Model& model, tinygltf::Mesh& mesh);
        AttribDatta getAttrib(tinygltf::Model& model, size_t &vecSize, int attribPos, bool calculatingPositions);
};