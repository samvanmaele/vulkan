#pragma once
#include <volk.h>

class DebugManager
{
    public:
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

        void init();
        void setupDebugMessenger(VkInstance &instance);
        void destroyDebugUtilsMessengerEXT(VkInstance instance, const VkAllocationCallbacks* pAllocator);

    private:
        VkDebugUtilsMessengerEXT debugMessenger;

        void checkValidationLayerSupport();
        void populateDebugMessengerCreateInfo();
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
        VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
};