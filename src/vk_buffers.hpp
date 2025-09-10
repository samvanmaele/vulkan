#pragma once
#include "common.hpp"

#include "vk_loadGLTF.hpp"

class BufferManager
{
    public:
        VkCommandPool commandPool;

        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;
        VkSampler textureSampler;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;

        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;

        struct alignas(16) UniformBufferObject
        {
            alignas(16) glm::mat4 model;
            alignas(16) glm::mat4 view;
            alignas(16) glm::mat4 proj;
        };

        Model testmodel;

        void init(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue);
        void createTextureImage(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, const char* filepath);
        void createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        void transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void createTextureImageView(VkDevice device);
        void createTextureSampler(VkPhysicalDevice physicalDevice, VkDevice device);
        void createDescriptorSetLayout(VkDevice device);
        void createUniformBuffers(VkPhysicalDevice physicalDevice, VkDevice device);
        void createDescriptorPool(VkDevice device);
        void createDescriptorSets(VkDevice device);
        void updateUniformBuffer(uint32_t currentFrame);
        void destroyAll(VkDevice device);
        void destroyTexture(VkDevice device);
        void destroyUniformBuffers(VkDevice device);

    private:
        void copyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void copyBuffer(VkDevice device, VkQueue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        VkCommandBuffer beginSingleTimeCommands(VkDevice &device);
        void endSingleTimeCommands(VkDevice &device, VkCommandBuffer &commandBuffer, VkQueue &graphicsQueue);
};