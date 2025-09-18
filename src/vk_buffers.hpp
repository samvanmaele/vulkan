#pragma once
#include "common.hpp"

#include "vk_loadGLTF.hpp"

class BufferManager
{
    public:
        VkCommandPool commandPool;

        VkSampler textureSampler;

        std::vector<VkBuffer> globalUniformBuffers;
        std::vector<VkDeviceMemory> globalUniformBuffersMemory;
        std::vector<void*> globalUniformBuffersMapped;

        VkDescriptorPool descriptorPool;
        VkDescriptorSetLayout globalDescriptorSetLayout;
        VkDescriptorSetLayout objectDescriptorSetLayout;
        std::vector<VkDescriptorSet> globalDescriptorSets;
        std::vector<VkDescriptorSet> objectDescriptorSets;
        std::vector<Model> models;

        std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts;
        std::array<std::vector<VkDescriptorSet>, 2> descriptorSets;

        void init(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue, std::vector<std::string> &modelPaths);
        void createTextureSampler(VkPhysicalDevice physicalDevice, VkDevice device);
        void createDescriptorSetLayout(VkDevice device);
        void createUniformBuffers(VkPhysicalDevice physicalDevice, VkDevice device);
        void createDescriptorPool(VkDevice device);
        void createDescriptorSets(VkDevice device);
        void updateUniformBuffer(uint32_t currentFrame);
        void destroyAll(VkDevice device);
        void destroyUniformBuffers(VkDevice device);

    private:
        void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
};