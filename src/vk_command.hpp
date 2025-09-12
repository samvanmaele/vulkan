#pragma once
#include "common.hpp"

class CommandManager
{
    public:
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;

        void init(VkDevice &device, uint32_t graphicsFamilyIndex, size_t swapChainSize, std::vector<VkFramebuffer> &swapChainFramebuffers, VkExtent2D &swapChainExtent, VkPipeline &graphicsPipeline, VkPipelineLayout pipelineLayout, VkRenderPass &renderPass, std::vector<VkDescriptorSet> descriptorSets, std::vector<PrimitiveData> primitiveDataList);
        void createCommandBuffers(VkDevice &device, size_t swapchainSize, std::vector<VkFramebuffer> &swapChainFramebuffers, VkExtent2D &swapChainExtent, VkPipeline &graphicsPipeline, VkPipelineLayout pipelineLayout, VkRenderPass &renderPass, std::vector<VkDescriptorSet> descriptorSets, std::vector<PrimitiveData> primitiveDataList);

    private:
        void createCommandPool(VkDevice &device, uint32_t graphicsFamilyIndex);
        void recordCommandBuffer(VkCommandBuffer &commandBuffer, VkFramebuffer &swapChainFramebuffer, VkExtent2D &swapChainExtent, VkPipeline &graphicsPipeline, VkPipelineLayout pipelineLayout, VkRenderPass &renderPass, VkDescriptorSet descriptorSet, std::vector<PrimitiveData> primitiveDataList);
};