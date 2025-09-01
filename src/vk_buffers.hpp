#pragma once
#include "common.hpp"

#include <tinygltf/tiny_gltf.h>

class BufferManager
{
    public:
        VkDescriptorSetLayout descriptorSetLayout;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;

        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;

        struct alignas(16) UniformBufferObject
        {
            alignas(16) glm::mat4 model;
            alignas(16) glm::mat4 view;
            alignas(16) glm::mat4 proj;
        };

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        void init(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue);
        void createDescriptorSetLayout(VkDevice device);
        void createVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue);
        void createIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue);
        void createUniformBuffers(VkPhysicalDevice physicalDevice, VkDevice device);
        void createDescriptorPool(VkDevice device);
        void createDescriptorSets(VkDevice device);
        void updateUniformBuffer(uint32_t currentFrame);

    private:
        void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void copyBuffer(VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        void Model(const char* filename);
        void bindNode(tinygltf::Model& model, const tinygltf::Node& node);
        void bindMesh(tinygltf::Model& model, tinygltf::Mesh& mesh);
};