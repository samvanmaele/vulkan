#include "vk_buffers.hpp"
#include "common.hpp"
#include <SDL3_image/SDL_image.h>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <stdexcept>

void BufferManager::init(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue, std::vector<std::string> &modelPaths)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueIndices.graphicsFamily.value();
    vk_check(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool), "failed to create command pool!");

    for (const auto& filename : modelPaths)
    {
        models.emplace_back(physicalDevice, device, graphicsQueue, commandPool, filename.c_str());
    }


    createDescriptorSetLayout(device);
    createTextureSampler(physicalDevice, device);
    createUniformBuffers(physicalDevice, device);
    createDescriptorPool(device);
    createDescriptorSets(device);
}
void BufferManager::createDescriptorSetLayout(VkDevice device)
{
    VkDescriptorSetLayoutBinding globalUboLayoutBinding{};
    globalUboLayoutBinding.binding = 0;
    globalUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    globalUboLayoutBinding.descriptorCount = 1;
    globalUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    globalUboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = static_cast<uint32_t>(models.size());
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {globalUboLayoutBinding, samplerLayoutBinding};

    std::vector<VkDescriptorBindingFlags> bindingFlags(2);
    bindingFlags[1] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsInfo{};
    bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    bindingFlagsInfo.pBindingFlags = bindingFlags.data();

    VkDescriptorSetLayoutCreateInfo globalLayoutInfo{};
    globalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    globalLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    globalLayoutInfo.pBindings = bindings.data();
    globalLayoutInfo.pNext = &bindingFlagsInfo;

    vk_check(vkCreateDescriptorSetLayout(device, &globalLayoutInfo, nullptr, &globalDescriptorSetLayout), "failed to create descriptor set layout!");

    VkDescriptorSetLayoutBinding objectUboLayoutBinding{};
    objectUboLayoutBinding.binding = 0;
    objectUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    objectUboLayoutBinding.descriptorCount = 1;
    objectUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo objectLayoutInfo{};
    objectLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    objectLayoutInfo.bindingCount = 1;
    objectLayoutInfo.pBindings = &objectUboLayoutBinding;

    vk_check(vkCreateDescriptorSetLayout(device, &objectLayoutInfo, nullptr, &objectDescriptorSetLayout), "failed to create object descriptor set layout!");

    descriptorSetLayouts = {globalDescriptorSetLayout, objectDescriptorSetLayout};
}
void BufferManager::createTextureSampler(VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    vk_check(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler), "failed to create texture sampler!");
}
void BufferManager::createUniformBuffers(VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkDeviceSize bufferSize = sizeof(GlobalUniformBufferObject);

    globalUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    globalUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    globalUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, globalUniformBuffers[i], globalUniformBuffersMemory[i]);
        vkMapMemory(device, globalUniformBuffersMemory[i], 0, bufferSize, 0, &globalUniformBuffersMapped[i]);
    }
}
void BufferManager::createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk_check(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer), "failed to create buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    vk_check(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory), "failed to allocate buffer memory!");
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
uint32_t BufferManager::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}
void BufferManager::createDescriptorPool(VkDevice device)
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + (MAX_FRAMES_IN_FLIGHT * models.size()));
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * models.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + (MAX_FRAMES_IN_FLIGHT * models.size()));

    vk_check(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool), "failed to create descriptor pool!");
}
void BufferManager::createDescriptorSets(VkDevice device)
{
    std::vector<VkDescriptorSetLayout> globalLayouts(MAX_FRAMES_IN_FLIGHT, globalDescriptorSetLayout);
    std::vector<uint32_t> descriptorCounts(MAX_FRAMES_IN_FLIGHT, static_cast<uint32_t>(models.size()));

    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT countInfo{};
    countInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
    countInfo.descriptorSetCount = static_cast<uint32_t>(globalLayouts.size());
    countInfo.pDescriptorCounts = descriptorCounts.data();

    VkDescriptorSetAllocateInfo globalAllocInfo{};
    globalAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    globalAllocInfo.descriptorPool = descriptorPool;
    globalAllocInfo.descriptorSetCount = static_cast<uint32_t>(globalLayouts.size());
    globalAllocInfo.pSetLayouts = globalLayouts.data();
    globalAllocInfo.pNext = &countInfo;

    globalDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    vk_check(vkAllocateDescriptorSets(device, &globalAllocInfo, globalDescriptorSets.data()), "failed to allocate global descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo globalUboInfo{};
        globalUboInfo.buffer = globalUniformBuffers[i];
        globalUboInfo.offset = 0;
        globalUboInfo.range = sizeof(GlobalUniformBufferObject);

        std::vector<VkDescriptorImageInfo> imageInfos(models.size());
        for (size_t j = 0; j < models.size(); j++)
        {
            imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[j].imageView = models[j].primitiveDataList[0].textureImageView;
            imageInfos[j].sampler = textureSampler;
        }

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = globalDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &globalUboInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = globalDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = static_cast<uint32_t>(imageInfos.size());
        descriptorWrites[1].pImageInfo = imageInfos.data();

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    size_t totalObjectSets = models.size() * MAX_FRAMES_IN_FLIGHT;
    objectDescriptorSets.resize(totalObjectSets);

    std::vector<VkDescriptorSetLayout> objectLayouts(totalObjectSets, objectDescriptorSetLayout);

    VkDescriptorSetAllocateInfo objectAllocInfo{};
    objectAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    objectAllocInfo.descriptorPool = descriptorPool;
    objectAllocInfo.descriptorSetCount = static_cast<uint32_t>(totalObjectSets);
    objectAllocInfo.pSetLayouts = objectLayouts.data();

    vk_check(vkAllocateDescriptorSets(device, &objectAllocInfo, objectDescriptorSets.data()), "failed to allocate object descriptor sets!");

    for (size_t i = 0; i < models.size(); i++)
    {
        for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
        {
            VkDescriptorBufferInfo objectBufferInfo{};
            objectBufferInfo.buffer = models[i].uniformBuffers[j];
            objectBufferInfo.offset = 0;
            objectBufferInfo.range = sizeof(ObjectUniformBufferObject);

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = objectDescriptorSets[i * MAX_FRAMES_IN_FLIGHT + j];
            write.dstBinding = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; write.descriptorCount = 1;
            write.pBufferInfo = &objectBufferInfo;
            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }
    }

    descriptorSets = {globalDescriptorSets, objectDescriptorSets};
}
void BufferManager::updateView(uint32_t currentFrame, glm::mat4 view)
{
    GlobalUniformBufferObject ubo{};
    ubo.view = view;
    ubo.proj = glm::perspective(glm::radians(45.0f), (float) WIDTH / (float) HEIGHT, 0.1f, 50.0f);
    ubo.proj[1][1] *= -1;
    memcpy(globalUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}
void BufferManager::updateUniformBuffer(uint32_t currentFrame)
{
    for (VkModel model : models)
    {
        memcpy(model.uniformBuffersMapped[currentFrame], &model.transmat, sizeof(glm::mat4));
    }
}

void BufferManager::destroyAll(VkDevice device)
{
    vkDestroySampler(device, textureSampler, nullptr);
    destroyUniformBuffers(device);

    for (auto& model : models)
    {
        model.destroyAll(device);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);
}
void BufferManager::destroyUniformBuffers(VkDevice device)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(device, globalUniformBuffers[i], nullptr);
        vkFreeMemory(device, globalUniformBuffersMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, globalDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, objectDescriptorSetLayout, nullptr);
}