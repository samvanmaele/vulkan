#include "vk_command.hpp"
#include "common.hpp"
#include <cstddef>
#include <vulkan/vulkan_core.h>

void CommandManager::init(VkDevice &device, uint32_t graphicsFamilyIndex, size_t swapChainSize, std::vector<VkFramebuffer> &swapChainFramebuffers, VkExtent2D &swapChainExtent, VkPipeline &graphicsPipeline, VkPipelineLayout pipelineLayout, VkPipeline skyboxPipeline, VkPipelineLayout skyboxPipelineLayout, VkBuffer skyboxPosBuffer, VkRenderPass &renderPass, std::array<std::vector<VkDescriptorSet>, 2> descriptorSets, const std::vector<VkModel>& models, VkModel player)
{
    createCommandPool(device, graphicsFamilyIndex);
    createCommandBuffers(device, swapChainSize, swapChainFramebuffers, swapChainExtent, graphicsPipeline, pipelineLayout, skyboxPipeline, skyboxPipelineLayout, skyboxPosBuffer, renderPass, descriptorSets, models, player);
}
void CommandManager::createCommandPool(VkDevice &device, uint32_t graphicsFamilyIndex)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamilyIndex;

    vk_check(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool), "failed to create command pool!");
}
void CommandManager::createCommandBuffers(VkDevice &device, size_t swapchainSize, std::vector<VkFramebuffer> &swapChainFramebuffers, VkExtent2D &swapChainExtent, VkPipeline &graphicsPipeline, VkPipelineLayout pipelineLayout, VkPipeline skyboxPipeline, VkPipelineLayout skyboxPipelineLayout, VkBuffer skyboxPosBuffer, VkRenderPass &renderPass, std::array<std::vector<VkDescriptorSet>, 2> descriptorSets, const std::vector<VkModel>& models, VkModel player)
{
    commandBuffers.resize(swapchainSize);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    vk_check(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()), "failed to allocate command buffers!");

    for (size_t i = 0; i < commandBuffers.size(); i++)
    {
        recordCommandBuffer(commandBuffers[i], swapChainFramebuffers[i], swapChainExtent, graphicsPipeline, pipelineLayout, skyboxPipeline, skyboxPipelineLayout, skyboxPosBuffer, renderPass, descriptorSets, models, player, i);
    }
}
void CommandManager::recordCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer swapChainFramebuffer, VkExtent2D swapChainExtent, VkPipeline graphicsPipeline, VkPipelineLayout pipelineLayout, VkPipeline skyboxPipeline, VkPipelineLayout skyboxPipelineLayout, VkBuffer skyboxPosBuffer, VkRenderPass renderPass, std::array<std::vector<VkDescriptorSet>, 2> descriptorSets, const std::vector<VkModel>& models, VkModel &player, size_t i)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    vk_check(vkBeginCommandBuffer(commandBuffer, &beginInfo), "failed to begin recording command buffer!");

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[0][i], 0, nullptr);

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    size_t playerObjectIndexBase = models.size() * MAX_FRAMES_IN_FLIGHT;
    for (const auto& primitive : player.primitiveDataList)
    {
        VkBuffer vertexBuffers[] = {primitive.positionBuffer, primitive.normalBuffer, primitive.texBuffer};
        VkDeviceSize offsets[] = {0, 0, 0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 3, vertexBuffers, offsets);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &descriptorSets[1][playerObjectIndexBase + i], 0, nullptr);

        if (primitive.indexCount > 0)
        {
            vkCmdBindIndexBuffer(commandBuffer, primitive.indexBuffer, 0, primitive.indexType);
            vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, 0, 0, 0);
        }
        else
        {
            vkCmdDraw(commandBuffer, primitive.vertexCount, 1, 0, 0);
        }
    }

    for (size_t j = 0; j < models.size(); j++)
    {
        for (const auto& primitive : models[j].primitiveDataList)
        {
            VkBuffer vertexBuffers[] = {primitive.positionBuffer, primitive.normalBuffer, primitive.texBuffer};
            VkDeviceSize offsets[] = {0, 0, 0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 3, vertexBuffers, offsets);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &descriptorSets[1][j * MAX_FRAMES_IN_FLIGHT + i], 0, nullptr);

            if (primitive.indexCount > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, primitive.indexBuffer, 0, primitive.indexType);
                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, 0, 0, 0);
            }
            else
            {
                vkCmdDraw(commandBuffer, primitive.vertexCount, 1, 0, 0);
            }
        }
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipelineLayout, 0, 1, &descriptorSets[0][i], 0, nullptr);

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer skyboxVertexBuffers[] = {skyboxPosBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, skyboxVertexBuffers, offsets);
    vkCmdDraw(commandBuffer, 36, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    vk_check(vkEndCommandBuffer(commandBuffer), "failed to record command buffer!");
}