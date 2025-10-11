#include "vk_objects.hpp"
#include "common.hpp"
#include "vk_buffers.hpp"
#include "vk_loadGLTF.hpp"
#include <SDL3_image/SDL_image.h>
#include <cstdint>
#include <cstring>
#include <vulkan/vulkan_core.h>

void ObjectManager::init(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue, std::vector<std::string> &modelPaths, std::string playerModelFile, std::array<float, 108> skyboxVertices, std::array<const char*, 6> skyboxPaths)
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
    player = VkModel(physicalDevice, device, graphicsQueue, commandPool, playerModelFile.c_str());
    createSkybox(physicalDevice, device, graphicsQueue, commandPool, skyboxPaths);
    BufferManager::stageBuffer(physicalDevice, device, graphicsQueue, commandPool, skyboxVertices.data(), sizeof(skyboxVertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, skyboxPositionBuffer, skyboxPositionBufferMemory);

    createDescriptorSetLayout(device);
    createTextureSampler(physicalDevice, device);
    createUniformBuffers(physicalDevice, device);
    createDescriptorPool(device);
    createDescriptorSets(device);
}
void ObjectManager::createSkybox(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, std::array<const char*, 6> skyboxPaths)
{
    SDL_Surface *faceImage = IMG_Load(skyboxPaths[0]);
    int width = faceImage->w;
    int height = faceImage->h;
    SDL_DestroySurface(faceImage);

    int channels = 4;
    VkDeviceSize imageSize = width * height * channels;
    VkDeviceSize totalImageSize = imageSize * 6;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManager::createBuffer(physicalDevice, device, totalImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, totalImageSize, 0, &data);

    for (int i = 0; i < 6; ++i)
    {
        SDL_Surface *image = IMG_Load(skyboxPaths[i]);
        SDL_Surface *rgbaSurface = SDL_ConvertSurface(image, SDL_PIXELFORMAT_RGBA32);
        memcpy(static_cast<char*>(data) + (i * imageSize), rgbaSurface->pixels, imageSize);
        SDL_DestroySurface(image);
        SDL_DestroySurface(rgbaSurface);
    }

    vkUnmapMemory(device, stagingBufferMemory);

    BufferManager::createImage(physicalDevice, device, width, height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, skyboxImage, skyboxImageMemory, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
    BufferManager::transitionImageLayout(device, graphicsQueue, commandPool, skyboxImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);
    BufferManager::copyBufferToImage(device, graphicsQueue, commandPool, stagingBuffer, skyboxImage, width, height, 6);
    BufferManager::transitionImageLayout(device, graphicsQueue, commandPool, skyboxImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);
    BufferManager::createImageView(device, skyboxImage, skyboxImageView, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE, 6);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
void ObjectManager::createDescriptorSetLayout(VkDevice device)
{
    VkDescriptorSetLayoutBinding globalUboLayoutBinding{};
    globalUboLayoutBinding.binding = 0;
    globalUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    globalUboLayoutBinding.descriptorCount = 1;
    globalUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    globalUboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding skyboxSamplerLayoutBinding{};
    skyboxSamplerLayoutBinding.binding = 1;
    skyboxSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    skyboxSamplerLayoutBinding.descriptorCount = 1;
    skyboxSamplerLayoutBinding.pImmutableSamplers = nullptr;
    skyboxSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {globalUboLayoutBinding, skyboxSamplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo globalLayoutInfo{};
    globalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    globalLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    globalLayoutInfo.pBindings = bindings.data();
    globalLayoutInfo.pNext = nullptr;

    vk_check(vkCreateDescriptorSetLayout(device, &globalLayoutInfo, nullptr, &globalDescriptorSetLayout), "failed to create descriptor set layout!");

    VkDescriptorSetLayoutBinding objectUboLayoutBinding{};
    objectUboLayoutBinding.binding = 0;
    objectUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    objectUboLayoutBinding.descriptorCount = 1;
    objectUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding objectSamplerLayoutBinding{};
    objectSamplerLayoutBinding.binding = 1;
    objectSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    objectSamplerLayoutBinding.descriptorCount = 1;
    objectSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> objectBindings = {objectUboLayoutBinding, objectSamplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo objectLayoutInfo{};
    objectLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    objectLayoutInfo.bindingCount = 2;
    objectLayoutInfo.pBindings = objectBindings.data();

    vk_check(vkCreateDescriptorSetLayout(device, &objectLayoutInfo, nullptr, &objectDescriptorSetLayout), "failed to create object descriptor set layout!");

    descriptorSetLayouts = {globalDescriptorSetLayout, objectDescriptorSetLayout};
}
void ObjectManager::createTextureSampler(VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
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
void ObjectManager::createUniformBuffers(VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkDeviceSize bufferSize = sizeof(GlobalUniformBufferObject);

    globalUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    globalUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    globalUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        BufferManager::createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, globalUniformBuffers[i], globalUniformBuffersMemory[i]);
        vkMapMemory(device, globalUniformBuffersMemory[i], 0, bufferSize, 0, &globalUniformBuffersMapped[i]);
    }
}
void ObjectManager::createDescriptorPool(VkDevice device)
{
    uint32_t numModelsIncludingPlayer = static_cast<uint32_t>(models.size() + 1);
    uint32_t totalObjectSets = numModelsIncludingPlayer * MAX_FRAMES_IN_FLIGHT;

    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + totalObjectSets);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + totalObjectSets);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + totalObjectSets);

    vk_check(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool), "failed to create descriptor pool!");
}
void ObjectManager::createDescriptorSets(VkDevice device)
{
    std::vector<VkDescriptorSetLayout> globalLayouts(MAX_FRAMES_IN_FLIGHT, globalDescriptorSetLayout);

    VkDescriptorSetAllocateInfo globalAllocInfo{};
    globalAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    globalAllocInfo.descriptorPool = descriptorPool;
    globalAllocInfo.descriptorSetCount = static_cast<uint32_t>(globalLayouts.size());
    globalAllocInfo.pSetLayouts = globalLayouts.data();

    globalDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    vk_check(vkAllocateDescriptorSets(device, &globalAllocInfo, globalDescriptorSets.data()), "failed to allocate global descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo globalUboInfo{};
        globalUboInfo.buffer = globalUniformBuffers[i];
        globalUboInfo.offset = 0;
        globalUboInfo.range = sizeof(GlobalUniformBufferObject);

        VkDescriptorImageInfo skyboxImageInfo{};
        skyboxImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        skyboxImageInfo.imageView = skyboxImageView;
        skyboxImageInfo.sampler = textureSampler;

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
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &skyboxImageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    size_t totalObjectSets = (models.size() + 1) * MAX_FRAMES_IN_FLIGHT;
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
            VkDescriptorSet currentSet = objectDescriptorSets[i * MAX_FRAMES_IN_FLIGHT + j];

            VkDescriptorBufferInfo objectBufferInfo{};
            objectBufferInfo.buffer = models[i].uniformBuffers[j];
            objectBufferInfo.offset = 0;
            objectBufferInfo.range = sizeof(ObjectUniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = models[i].primitiveDataList[0].textureImageView;
            imageInfo.sampler = textureSampler;

            std::array<VkWriteDescriptorSet, 2> writes{};

            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = currentSet;
            writes[0].dstBinding = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[0].descriptorCount = 1;
            writes[0].pBufferInfo = &objectBufferInfo;

            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = currentSet;
            writes[1].dstBinding = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[1].descriptorCount = 1;
            writes[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        }
    }

    size_t playerBaseIndex = models.size() * MAX_FRAMES_IN_FLIGHT;
    for (size_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++)
    {
        VkDescriptorSet currentSet = objectDescriptorSets[playerBaseIndex + f];

        VkDescriptorBufferInfo objectBufferInfo{};
        objectBufferInfo.buffer = player.uniformBuffers[f];
        objectBufferInfo.offset = 0;
        objectBufferInfo.range = sizeof(ObjectUniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = player.primitiveDataList[0].textureImageView;
        imageInfo.sampler = textureSampler;

        std::array<VkWriteDescriptorSet, 2> writes{};

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = currentSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &objectBufferInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = currentSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    descriptorSets = {globalDescriptorSets, objectDescriptorSets};
}
void ObjectManager::updateView(uint32_t currentFrame, glm::mat4 view)
{
    GlobalUniformBufferObject ubo{};
    ubo.view = view;
    ubo.proj = glm::perspective(glm::radians(45.0f), (float) WIDTH / (float) HEIGHT, 0.1f, 50.0f);
    ubo.proj[1][1] *= -1;
    memcpy(globalUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}
void ObjectManager::updateUniformBuffer(uint32_t currentFrame)
{
    for (VkModel model : models)
    {
        memcpy(model.uniformBuffersMapped[currentFrame], &model.transmat, sizeof(glm::mat4));
    }
    memcpy(player.uniformBuffersMapped[currentFrame], &player.transmat, sizeof(glm::mat4));
}

void ObjectManager::destroyAll(VkDevice device)
{
    vkDestroySampler(device, textureSampler, nullptr);
    destroyUniformBuffers(device);

    for (auto& model : models)
    {
        model.destroyAll(device);
    }
    player.destroyAll(device);

    vkDestroyBuffer(device, skyboxPositionBuffer, nullptr);
    vkFreeMemory(device, skyboxPositionBufferMemory, nullptr);

    vkDestroyImageView(device, skyboxImageView, nullptr);
    vkDestroyImage(device, skyboxImage, nullptr);
    vkFreeMemory(device, skyboxImageMemory, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);
}
void ObjectManager::destroyUniformBuffers(VkDevice device)
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