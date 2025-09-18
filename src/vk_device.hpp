#pragma once
#include <SDL3/SDL_video.h>
#include "common.hpp"

class DeviceManager
{
    public:
        VkInstance instance;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        QueueFamilyIndices indices;
        SwapChainSupportDetails swapChainSupport;

        void createInstance(SDL_Window* window, VkDebugUtilsMessengerCreateInfoEXT &debugCreateInfo);
        bool init();
        void reinit();
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice);

    private:
        const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME};

        std::vector<const char*> getRequiredExtensions(SDL_Window* window);
        bool checkPhysicalDevice();
        int rateDeviceSuitability(VkPhysicalDevice physicalDevice);
        bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);
        void createLogicalDevice(QueueFamilyIndices &indices);

};