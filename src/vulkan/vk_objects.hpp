#pragma once
#include "common.hpp"
#include "vk_loadGLTF.hpp"

class ObjectManager
{
    public:
        VkCommandPool commandPool;

        VkImage skyboxImage;
        VkDeviceMemory skyboxImageMemory;
        VkImageView skyboxImageView;

        VkBuffer skyboxPositionBuffer;
        VkDeviceMemory skyboxPositionBufferMemory;

        VkSampler textureSampler;

        std::vector<VkBuffer> globalUniformBuffers;
        std::vector<VkDeviceMemory> globalUniformBuffersMemory;
        std::vector<void*> globalUniformBuffersMapped;

        VkDescriptorPool descriptorPool;
        VkDescriptorSetLayout globalDescriptorSetLayout;
        VkDescriptorSetLayout objectDescriptorSetLayout;
        std::vector<VkDescriptorSet> globalDescriptorSets;
        std::vector<VkDescriptorSet> objectDescriptorSets;
        std::vector<VkModel> models;
        VkModel player;

        std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts;
        std::array<std::vector<VkDescriptorSet>, 2> descriptorSets;

        void init(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue, std::vector<std::string> &modelPaths, std::string playerModelFile, std::array<float, 108> skyboxVertices, std::array<const char*, 6> skyboxPaths);
        void createSkybox(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, std::array<const char*, 6> skyboxPaths);
        void createTextureSampler(VkPhysicalDevice physicalDevice, VkDevice device);
        void createDescriptorSetLayout(VkDevice device);
        void createUniformBuffers(VkPhysicalDevice physicalDevice, VkDevice device);
        void createDescriptorPool(VkDevice device);
        void createDescriptorSets(VkDevice device);
        void updateView(uint32_t currentFrame, glm::mat4 view);
        void updateUniformBuffer(uint32_t currentFrame);
        void destroyAll(VkDevice device);
        void destroyUniformBuffers(VkDevice device);
};