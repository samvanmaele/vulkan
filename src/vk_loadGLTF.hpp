#pragma once
#include "common.hpp"
#include <tinygltf/tiny_gltf.h>

struct AttribDatta
{
    const unsigned char* dataPtr;
    int stride;
};

class VkModel
{
    public:
        struct PrimitiveData
        {
            size_t vertexCount, indexCount;
            VkIndexType indexType;

            VkBuffer posBuffer;
            VkDeviceMemory positionBufferMemory;
            VkBuffer normalBuffer;
            VkDeviceMemory normalBufferMemory;
            VkBuffer texBuffer;
            VkDeviceMemory texBufferMemory;
            VkBuffer indexBuffer;
            VkDeviceMemory indexBufferMemory;

            VkImage textureImage;
            VkDeviceMemory textureImageMemory;
            VkImageView textureImageView;
            uint32_t textureIndex;

            void destroyTexture(VkDevice device)
            {
                vkDestroyImageView(device, textureImageView, nullptr);
                vkDestroyImage(device, textureImage, nullptr);
                vkFreeMemory(device, textureImageMemory, nullptr);
            }
            void destroyVertexBuffers(VkDevice device)
            {
                vkDestroyBuffer(device, posBuffer, nullptr);
                vkFreeMemory(device, positionBufferMemory, nullptr);
                vkDestroyBuffer(device, normalBuffer, nullptr);
                vkFreeMemory(device, normalBufferMemory, nullptr);
                vkDestroyBuffer(device, texBuffer, nullptr);
                vkFreeMemory(device, texBufferMemory, nullptr);
                vkDestroyBuffer(device, indexBuffer, nullptr);
                vkFreeMemory(device, indexBufferMemory, nullptr);
            }
        };

        std::vector<PrimitiveData> primitiveDataList;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;

        VkModel() = default;
        VkModel(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, const char* filename);

        void init(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue);
        void createTextureImage(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage &textureImage, VkDeviceMemory &textureImageMemory, const unsigned char* pixels, int texWidth, int texHeight);
        void createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        void transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void createTextureImageView(VkDevice device, VkImageView &textureImageView, VkImage textureImage);

        void destroyAll(VkDevice device);

    private:
        void copyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
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