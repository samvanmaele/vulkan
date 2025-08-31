#pragma once
#include "common.hpp"

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

        struct UniformBufferObject
        {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
        };

        void createDescriptorSetLayout(VkDevice device);

        const std::vector<Vertex> vertices
        {
            {{0, 0}, {1.0f, 1.0f, 1.0f}},
            {{rotX(0.000000f), rotY(0.000000f)}, {1.0f,        0.00000000f, 0.00000000f}},
            {{rotX(0.523598f), rotY(0.523598f)}, {3.0f / 4.0f, 1.0f / 4.0f, 0.00000000f}},
            {{rotX(1.047197f), rotY(1.047197f)}, {1.0f / 2.0f, 1.0f / 2.0f, 0.00000000f}},
            {{rotX(1.570796f), rotY(1.570796f)}, {1.0f / 4.0f, 3.0f / 4.0f, 0.00000000f}},
            {{rotX(2.094395f), rotY(2.094395f)}, {0.00000000f, 1.0f,        0.00000000f}},
            {{rotX(2.617993f), rotY(2.617993f)}, {0.00000000f, 3.0f / 4.0f, 1.0f / 4.0f}},
            {{rotX(3.141592f), rotY(3.141592f)}, {0.00000000f, 1.0f / 2.0f, 1.0f / 2.0f}},
            {{rotX(3.665191f), rotY(3.665191f)}, {0.00000000f, 1.0f / 4.0f, 3.0f / 4.0f}},
            {{rotX(4.188790f), rotY(4.188790f)}, {0.00000000f, 0.00000000f, 1.0f        }},
            {{rotX(4.712388f), rotY(4.712388f)}, {1.0f / 4.0f, 0.00000000f, 3.0f / 4.0f}},
            {{rotX(5.235987f), rotY(5.235987f)}, {1.0f / 2.0f, 0.00000000f, 1.0f / 2.0f}},
            {{rotX(5.759586f), rotY(5.759586f)}, {3.0f / 4.0f, 0.00000000f, 1.0f / 4.0f}},
        };
        const std::vector<uint16_t> indices
        {
            0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 7, 0, 7, 8, 0, 8, 9, 0, 9, 10, 0, 10, 11, 0, 11, 12, 0, 12, 1
        };

        void createVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue);
        void createIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue);
        void createUniformBuffers(VkPhysicalDevice physicalDevice, VkDevice device);
        void createDescriptorPool(VkDevice device);
        void createDescriptorSets(VkDevice device);
        void updateUniformBuffer(uint32_t currentFrame);

    private:
        float rotX(float x);
        float rotY(float y);

        void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void copyBuffer(VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
};