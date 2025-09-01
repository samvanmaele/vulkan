#include "vk_buffers.hpp"
#include "common.hpp"
#include <cstring>
#include <chrono>
#include <iostream>

void BufferManager::init(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue)
{
    Model("models/vedal987/vedal987.gltf");
    createDescriptorSetLayout(device);
    createVertexBuffer(physicalDevice, device, queueIndices, graphicsQueue);
    createIndexBuffer(physicalDevice, device, queueIndices, graphicsQueue);
    createUniformBuffers(physicalDevice, device);
    createDescriptorPool(device);
    createDescriptorSets(device);
}
void BufferManager::createDescriptorSetLayout(VkDevice device)
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;
    vk_check(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout), "failed to create descriptor set layout!");
}
void BufferManager::createVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue)
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    copyBuffer(device, queueIndices, graphicsQueue, stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
void BufferManager::createIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(device, queueIndices, graphicsQueue, stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
void BufferManager::createUniformBuffers(VkPhysicalDevice physicalDevice, VkDevice device)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}
void BufferManager::createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk_check(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer), "failed to create vertex buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    vk_check(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory), "failed to allocate vertex buffer memory!");
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
void BufferManager::copyBuffer(VkDevice device, QueueFamilyIndices queueIndices, VkQueue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueIndices.graphicsFamily.value();

    VkCommandPool commandPool;
    vk_check(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool), "failed to create command pool!");

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandPool, nullptr);
}
void BufferManager::createDescriptorPool(VkDevice device)
{
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    vk_check(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool), "failed to create descriptor pool!");
}
void BufferManager::createDescriptorSets(VkDevice device)
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    vk_check(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()), "failed to allocate descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr; // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
}
void BufferManager::updateUniformBuffer(uint32_t currentFrame)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), WIDTH / (float) HEIGHT, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}
void BufferManager::Model(const char* filename)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!warn.empty()) {std::cout << "Warning: " << warn << std::endl;}
    if (!err.empty()) {std::cerr << "Error: " << err << std::endl;}
    if (!res) {std::cerr << "Failed to load glTF: " << filename << std::endl;}

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];

    for (int nodeIndex : scene.nodes)
    {
        const tinygltf::Node& node = model.nodes[nodeIndex];
        bindNode(model, node);
    }
}
void BufferManager::bindNode(tinygltf::Model& model, const tinygltf::Node& node)
{
    if (node.mesh >= 0) {bindMesh(model, model.meshes[node.mesh]);}
    for (int child : node.children)
    {
        if (child >= 0) {bindNode(model, model.nodes[child]);}
    }
}
void BufferManager::bindMesh(tinygltf::Model& model, tinygltf::Mesh& mesh)
{
    for (const auto& primitive : mesh.primitives)
    {
        for (const auto& attrib : primitive.attributes)
        {
            if (attrib.first == "POSITION")
            {
                const auto& vertexAccessor = model.accessors[attrib.second];
                const auto& vertexBufferView = model.bufferViews[vertexAccessor.bufferView];
                const auto& vertexBuffer = model.buffers[vertexBufferView.buffer];

                const unsigned char* dataPtrVertex = vertexBuffer.data.data() + vertexBufferView.byteOffset + vertexAccessor.byteOffset;
                size_t vertexCount = vertexAccessor.count;
                if (vertices.size() == 0) vertices.resize(vertexCount);
                size_t stride = vertexAccessor.ByteStride(vertexBufferView);
                if (stride == 0) stride = sizeof(float) * 3;

                for (size_t i = 0; i < vertexCount; i++)
                {
                    const float* pos = reinterpret_cast<const float*>(dataPtrVertex + i * stride);
                    vertices[i].pos = glm::vec3(pos[0], pos[1], pos[2]);
                    vertices[i].color = glm::vec3(1.0f, 0.0f, 0.0f);
                }
            }
            else if (attrib.first == "NORMAL")
            {
                const auto& normalAccessor = model.accessors[attrib.second];
                const auto& normalBufferView = model.bufferViews[normalAccessor.bufferView];
                const auto& normalBuffer = model.buffers[normalBufferView.buffer];

                const unsigned char* dataPtrNormal = normalBuffer.data.data() + normalBufferView.byteOffset + normalAccessor.byteOffset;
                size_t normalCount = normalAccessor.count;
                if (vertices.size() == 0) vertices.resize(normalCount);
                size_t stride = normalAccessor.ByteStride(normalBufferView);
                if (stride == 0) stride = sizeof(float) * 3;

                for (size_t i = 0; i < normalCount; i++)
                {
                    const float* norm = reinterpret_cast<const float*>(dataPtrNormal + i * stride);
                    vertices[i].normal = glm::vec3(norm[0], norm[1], norm[2]);
                }
            }
        }
        const auto& indexAccessor = model.accessors[primitive.indices];
        const auto& indexBufferView = model.bufferViews[indexAccessor.bufferView];
        const auto& indexBuffer = model.buffers[indexBufferView.buffer];

        const unsigned char* dataPtrIndex = indexBuffer.data.data() + indexBufferView.byteOffset + indexAccessor.byteOffset;
        size_t indexCount = indexAccessor.count;
        indices.resize(indexCount);

        int stride = 0;
        switch (indexAccessor.componentType)
        {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  stride = 1; break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: stride = 2; break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   stride = 4; break;
            default: throw std::runtime_error("Unsupported index type");
        }

        for (size_t i = 0; i < indexCount; i++)
        {
            uint32_t value = 0;
            memcpy(&value, dataPtrIndex + i * stride, stride);
            indices[i] = value;
        }
    }
}