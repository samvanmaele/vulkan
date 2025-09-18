#pragma once
#include "SDL3/SDL_video.h"
#include "common.hpp"

class FrameManager
{
    public:
        std::vector<VkImage> swapChainImages;
        VkExtent2D swapChainExtent;
        VkSwapchainKHR swapChain;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        VkPipeline graphicsPipeline;
        VkPipelineLayout pipelineLayout;
        VkRenderPass renderPass;

        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;
        VkFormat depthFormat;

        void init(VkPhysicalDevice physicalDevice, VkDevice &device, SDL_Window* window, VkSurfaceKHR &surface, QueueFamilyIndices &indices, VkQueue graphicsQueue, SwapChainSupportDetails &swapChainSupport, std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts);
        void reinit(VkPhysicalDevice physicalDevice, VkDevice &device, SDL_Window* window, VkSurfaceKHR &surface, QueueFamilyIndices &indices, VkQueue graphicsQueue, SwapChainSupportDetails &swapChainSupport);
        void createSwapChain(VkDevice &device, SDL_Window* window, VkSurfaceKHR &surface, QueueFamilyIndices &indices, SwapChainSupportDetails &swapChainSupport);
        void createImageViews(VkDevice &device);
        void createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
        VkImageView createImageView(VkDevice &device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        void transitionImageLayout(VkDevice device, uint32_t graphicsFamilyIndex, VkQueue graphicsQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        VkCommandBuffer beginSingleTimeCommands(VkDevice &device, VkCommandPool commandPool);
        void endSingleTimeCommands(VkDevice &device, VkCommandBuffer &commandBuffer, VkQueue &graphicsQueue, VkCommandPool commandPool);
        void createDepthResources(VkPhysicalDevice physicalDevice, VkDevice &device, uint32_t graphicsFamilyIndex, VkQueue graphicsQueue);
        VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        void createRenderPass(VkDevice &device);
        void createGraphicsPipeline(VkDevice &device, std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts);
        void createFramebuffers(VkDevice &device);
        void cleanupSwapChain(VkDevice &device);
        void cleanupPipeline(VkDevice &device);

    private:
        VkFormat swapChainImageFormat;
        std::vector<VkImageView> swapChainImageViews;

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(SDL_Window* window, const VkSurfaceCapabilitiesKHR& capabilities);
        std::vector<char> readFile(const std::string& filename);
        VkShaderModule createShaderModule(VkDevice &device, const std::vector<char>& code);
};