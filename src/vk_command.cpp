#include "vk_command.hpp"

void CommandManager::init(VkDevice &device, QueueFamilyIndices &indices, size_t swapChainSize, std::vector<VkFramebuffer> &swapChainFramebuffers, VkExtent2D &swapChainExtent, VkPipeline &graphicsPipeline, VkPipelineLayout pipelineLayout, VkRenderPass &renderPass, VkBuffer vertexBuffer, VkBuffer indexBuffer, size_t indicesSize, std::vector<VkDescriptorSet> descriptorSets)
{
    createCommandPool(device, indices);
    createCommandBuffers(device, swapChainSize, swapChainFramebuffers, swapChainExtent, graphicsPipeline, pipelineLayout, renderPass, vertexBuffer, indexBuffer, indicesSize, descriptorSets);
}
void CommandManager::createCommandPool(VkDevice &device, QueueFamilyIndices &indices)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    vk_check(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool), "failed to create command pool!");
}
void CommandManager::createCommandBuffers(VkDevice &device, size_t swapchainSize, std::vector<VkFramebuffer> &swapChainFramebuffers, VkExtent2D &swapChainExtent, VkPipeline &graphicsPipeline, VkPipelineLayout pipelineLayout, VkRenderPass &renderPass, VkBuffer vertexBuffer, VkBuffer indexBuffer, size_t indicesSize, std::vector<VkDescriptorSet> descriptorSets)
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
        recordCommandBuffer(commandBuffers[i], swapChainFramebuffers[i], swapChainExtent, graphicsPipeline, pipelineLayout, renderPass, vertexBuffer, indexBuffer, indicesSize, descriptorSets[i]);
    }
}
void CommandManager::recordCommandBuffer(VkCommandBuffer &commandBuffer, VkFramebuffer &swapChainFramebuffer, VkExtent2D &swapChainExtent, VkPipeline &graphicsPipeline, VkPipelineLayout pipelineLayout, VkRenderPass &renderPass, VkBuffer vertexBuffer, VkBuffer indexBuffer, size_t indicesSize, VkDescriptorSet descriptorSet)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    vk_check(vkBeginCommandBuffer(commandBuffer, &beginInfo), "failed to begin recording command buffer!");

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indicesSize), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    vk_check(vkEndCommandBuffer(commandBuffer), "failed to record command buffer!");
}