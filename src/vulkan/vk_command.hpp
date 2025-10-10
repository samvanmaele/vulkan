#pragma once
#include "common.hpp"
#include "vk_loadGLTF.hpp"

class CommandManager
{
    public:
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;

        void init(VkDevice &device, uint32_t graphicsFamilyIndex, size_t swapChainSize, std::vector<VkFramebuffer> &swapChainFramebuffers, VkExtent2D &swapChainExtent, VkPipeline &graphicsPipeline, VkPipelineLayout pipelineLayout, VkPipeline skyboxPipeline, VkPipelineLayout skyboxPipelineLayout, VkBuffer skyboxPosBuffer, VkRenderPass &renderPass, std::array<std::vector<VkDescriptorSet>, 2> descriptorSets, const std::vector<VkModel>& models, VkModel player);
        void createCommandBuffers(VkDevice &device, size_t swapchainSize, std::vector<VkFramebuffer> &swapChainFramebuffers, VkExtent2D &swapChainExtent, VkPipeline &graphicsPipeline, VkPipelineLayout pipelineLayout, VkPipeline skyboxPipeline, VkPipelineLayout skyboxPipelineLayout, VkBuffer skyboxPosBuffer, VkRenderPass &renderPass, std::array<std::vector<VkDescriptorSet>, 2> descriptorSets, const std::vector<VkModel>& models, VkModel player);

    private:
        void createCommandPool(VkDevice &device, uint32_t graphicsFamilyIndex);
        void recordCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer swapChainFramebuffer, VkExtent2D swapChainExtent, VkPipeline graphicsPipeline, VkPipelineLayout pipelineLayout, VkPipeline skyboxPipeline, VkPipelineLayout skyboxPipelineLayout, VkBuffer skyboxPosBuffer, VkRenderPass renderPass, std::array<std::vector<VkDescriptorSet>, 2> descriptorSets, const std::vector<VkModel>& models, VkModel &player, size_t i);
};