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

        void init(VkDevice &device, SDL_Window* window, VkSurfaceKHR &surface, QueueFamilyIndices &indices, SwapChainSupportDetails &swapChainSupport, VkDescriptorSetLayout descriptorSetLayout);
        void reinit(VkDevice &device, SDL_Window* window, VkSurfaceKHR &surface, QueueFamilyIndices &indices, SwapChainSupportDetails &swapChainSupport);
        void createSwapChain(VkDevice &device, SDL_Window* window, VkSurfaceKHR &surface, QueueFamilyIndices &indices, SwapChainSupportDetails &swapChainSupport);
        void createImageViews(VkDevice &device);
        void createRenderPass(VkDevice &device);
        void createGraphicsPipeline(VkDevice &device, VkDescriptorSetLayout descriptorSetLayout);
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