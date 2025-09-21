#pragma once
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <string>
#include <volk.h>
#include <optional>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <vulkan/vulkan_core.h>

const int MAX_FRAMES_IN_FLIGHT = 3;
extern int WIDTH;
extern int HEIGHT;

const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

void vk_check(VkResult result, const std::string& msg);

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

static std::array<VkVertexInputBindingDescription, 3> getBindingDescription()
{
    std::array<VkVertexInputBindingDescription, 3> bindings{};

    bindings[0].binding = 0;
    bindings[0].stride = sizeof(glm::vec3);
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    bindings[1].binding = 1;
    bindings[1].stride = sizeof(glm::vec3);
    bindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    bindings[2].binding = 2;
    bindings[2].stride = sizeof(glm::vec2);
    bindings[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindings;
}
static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    attributeDescriptions[1].binding = 1;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = 0;

    attributeDescriptions[2].binding = 2;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = 0;

    return attributeDescriptions;
}

struct GlobalUniformBufferObject
{
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
struct ObjectUniformBufferObject
{
    alignas(16) glm::mat4 model;
};