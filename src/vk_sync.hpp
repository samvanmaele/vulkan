#pragma once
#include <volk.h>
#include <vector>

class SyncManager
{
    public:
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        void createSyncObjects(VkDevice &device);
        void cleanupSyncObjects(VkDevice &device);
};