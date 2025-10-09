#pragma once
#include "SDL3/SDL_video.h"
#include "common.hpp"

class FrameManager
{
    public:
        VkCommandPool commandPool;

        std::vector<VkImage> swapChainImages;
        VkExtent2D swapChainExtent;
        VkSwapchainKHR swapChain;
        std::vector<VkFramebuffer> swapChainFramebuffers;

        VkPipeline graphicsPipeline;
        VkPipelineLayout pipelineLayout;
        VkPipeline skyboxGraphicsPipeline;
        VkPipelineLayout skyboxPipelineLayout;

        VkRenderPass renderPass;

        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;
        VkFormat depthFormat;

        void init(VkPhysicalDevice physicalDevice, VkDevice &device, SDL_Window* window, VkSurfaceKHR &surface, QueueFamilyIndices &indices, VkQueue graphicsQueue, SwapChainSupportDetails &swapChainSupport, std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts);
        void reinit(VkPhysicalDevice physicalDevice, VkDevice &device, SDL_Window* window, VkSurfaceKHR &surface, QueueFamilyIndices &indices, VkQueue graphicsQueue, SwapChainSupportDetails &swapChainSupport);
        void createSwapChain(VkDevice &device, SDL_Window* window, VkSurfaceKHR &surface, QueueFamilyIndices &indices, SwapChainSupportDetails &swapChainSupport);
        void createImageViews(VkDevice &device);
        void createDepthResources(VkPhysicalDevice physicalDevice, VkDevice &device, uint32_t graphicsFamilyIndex, VkQueue graphicsQueue);
        VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        void createRenderPass(VkDevice &device);
        void createGraphicsPipeline(VkDevice &device, std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts, const std::string vertexPath, const std::string fragmentPath);
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