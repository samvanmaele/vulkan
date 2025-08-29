#include <SDL3/SDL_stdinc.h>
#include <cmath>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif __linux__
    #include <sched.h>
#endif
#include <thread>

#include <volk.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_video.h>
#include <stdexcept>
#include <SDL3/SDL.h>

#include <cstdlib>
#include <vector>
#include <cstdint>
#include <atomic>

#include "common_structs.hpp"
#include "vk_debug.hpp"
#include "vk_device.hpp"
#include "vk_frames.hpp"
#include "vk_command.hpp"
#include "vk_sync.hpp"

class Triangle
{
    public:
        Triangle()
        {
            initWindow();

            #ifdef __EMSCRIPTEN__
                initWebGL();
                mainLoop();
                cleanAllWebGPU();
            #else
                if (!initVulkan())
                {
                    printf("Failed to create vulkan instance\n");
                    cleanInstance();
                    //init openGL
                }
                else
                {
                    mainLoop();
                    cleanAll();
                }
            #endif
        }

    private:
        SDL_Window* window;

        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

        DebugManager debugManager;
        DeviceManager deviceManager;
        FrameManager frameManager;
        CommandManager commandManager;
        SyncManager syncManager;

        void initWindow()
        {
            SDL_Init(SDL_INIT_VIDEO);

            #ifdef __EMSCRIPTEN__
                window = SDL_CreateWindow("...", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
            #else
                window = SDL_CreateWindow("...", WIDTH, HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
            #endif
        }
        bool initVulkan()
        {
            volkInitialize();

            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
            if (enableValidationLayers && !debugManager.checkValidationLayerSupport()) throw std::runtime_error("validation layers requested, but not available!");
            if (enableValidationLayers) debugManager.populateDebugMessengerCreateInfo(debugCreateInfo);

            deviceManager.createInstance(window, debugCreateInfo);

            volkLoadInstance(deviceManager.instance);

            if (enableValidationLayers) debugManager.setupDebugMessenger(deviceManager.instance);
            if (!deviceManager.checkPhysicalDevice() || forceOpenGL) return false;

            deviceManager.createLogicalDevice(deviceManager.indices);

            SwapChainSupportDetails swapChainSupport = deviceManager.querySwapChainSupport(deviceManager.physicalDevice);
            frameManager.init(deviceManager.device, window, deviceManager.surface, deviceManager.indices, swapChainSupport);

            createVertexBuffer();
            createIndexBuffer();

            commandManager.init(deviceManager.device, deviceManager.indices, frameManager.swapChainImages.size(), frameManager.swapChainFramebuffers, frameManager.swapChainExtent, frameManager.graphicsPipeline, frameManager.renderPass, vertexBuffer, indexBuffer, indices.size());
            syncManager.createSyncObjects(deviceManager.device);

            return true;
        }
        void recreateSwapChain()
        {
            vkDeviceWaitIdle(deviceManager.device);

            SwapChainSupportDetails swapChainSupport = deviceManager.querySwapChainSupport(deviceManager.physicalDevice);
            frameManager.reinit(deviceManager.device, window, deviceManager.surface, deviceManager.indices, swapChainSupport);

            vkFreeCommandBuffers(deviceManager.device, commandManager.commandPool, static_cast<uint32_t>(commandManager.commandBuffers.size()), commandManager.commandBuffers.data());
            commandManager.createCommandBuffers(deviceManager.device, frameManager.swapChainImages.size(), frameManager.swapChainFramebuffers, frameManager.swapChainExtent, frameManager.graphicsPipeline, frameManager.renderPass, vertexBuffer, indexBuffer, indices.size());
        }

        float rotX(float x)
        {
            return (std::cos(x) - std::sin(x)) * 0.5f;
        }
        float rotY(float y)
        {
            return (std::cos(y) + std::sin(y)) * 0.5f;
        }

        const std::vector<Vertex> vertices =
        {
            {{0, 0}, {1.0f, 0.0f, 0.0f}},
            {{rotX(0.0f), rotY(0.0f)}, {1.0f, 0.0f, 0.0f}},
            {{rotX(0.523598f), rotY(0.523598f)}, {1.0f, 0.0f, 0.0f}},
            {{rotX(1.047197f), rotY(1.047197f)}, {1.0f, 0.0f, 0.0f}},
            {{rotX(1.570796f), rotY(1.570796f)}, {1.0f, 0.0f, 0.0f}},
            {{rotX(2.094395f), rotY(2.094395f)}, {1.0f, 0.0f, 0.0f}},
            {{rotX(2.617993f), rotY(2.617993f)}, {1.0f, 0.0f, 0.0f}},
            {{rotX(3.141592f), rotY(3.141592f)}, {1.0f, 0.0f, 0.0f}},
            {{rotX(3.665191f), rotY(3.665191f)}, {1.0f, 0.0f, 0.0f}},
            {{rotX(4.188790f), rotY(4.188790f)}, {1.0f, 0.0f, 0.0f}},
            {{rotX(4.712388f), rotY(4.712388f)}, {1.0f, 0.0f, 0.0f}},
            {{rotX(5.235987f), rotY(5.235987f)}, {1.0f, 0.0f, 0.0f}},
            {{rotX(5.759586f), rotY(5.759586f)}, {1.0f, 0.0f, 0.0f}},
        };
        const std::vector<uint16_t> indices =
        {
            0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 7, 0, 7, 8, 0, 8, 9, 0, 9, 10, 0, 10, 11, 0, 11, 12, 0, 12, 1
        };

        void createVertexBuffer()
        {
            VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void* data;
            vkMapMemory(deviceManager.device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, vertices.data(), (size_t) bufferSize);
            vkUnmapMemory(deviceManager.device, stagingBufferMemory);

            createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
            copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

            vkDestroyBuffer(deviceManager.device, stagingBuffer, nullptr);
            vkFreeMemory(deviceManager.device, stagingBufferMemory, nullptr);
        }
        void createIndexBuffer()
        {
            VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void* data;
            vkMapMemory(deviceManager.device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, indices.data(), (size_t) bufferSize);
            vkUnmapMemory(deviceManager.device, stagingBufferMemory);

            createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

            copyBuffer(stagingBuffer, indexBuffer, bufferSize);

            vkDestroyBuffer(deviceManager.device, stagingBuffer, nullptr);
            vkFreeMemory(deviceManager.device, stagingBufferMemory, nullptr);
        }
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(deviceManager.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) throw std::runtime_error("failed to create vertex buffer!");

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(deviceManager.device, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(deviceManager.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) throw std::runtime_error("failed to allocate vertex buffer memory!");
            vkBindBufferMemory(deviceManager.device, buffer, bufferMemory, 0);
        }
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
        {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(deviceManager.physicalDevice, &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
            {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }
            throw std::runtime_error("failed to find suitable memory type!");
        }
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
        {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = deviceManager.indices.graphicsFamily.value();

            VkCommandPool commandPool;
            if (vkCreateCommandPool(deviceManager.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) throw std::runtime_error("failed to create command pool!");

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(deviceManager.device, &allocInfo, &commandBuffer);

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

            vkQueueSubmit(deviceManager.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(deviceManager.graphicsQueue);

            vkFreeCommandBuffers(deviceManager.device, commandPool, 1, &commandBuffer);
            vkDestroyCommandPool(deviceManager.device, commandPool, nullptr);
        }

        std::atomic<bool> running = true;
        std::atomic<bool> resized = false;
        std::atomic<uint32_t> frameCount = 0;
        std::thread renderThread;
        std::thread windowThread;

        void setThreadAffinityAndPriority()
        {
            #ifdef _WIN32
                HANDLE hThread = GetCurrentThread();
                SetThreadAffinityMask(hThread, 1 << 1);
                SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
            #elif __linux__
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                const int core_id = 19;
                CPU_SET(core_id, &cpuset);

                const pthread_t thread = pthread_self();
                pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

                sched_param sch_params;
                sch_params.sched_priority = sched_get_priority_max(SCHED_RR);
                pthread_setschedparam(thread, SCHED_RR, &sch_params);
            #endif
        }
        void mainLoop()
        {
            createRenderthread();
            setThreadAffinityAndPriority();

            uint32_t lastTime = SDL_GetTicks();
            char titleBuffer[64];

            const int targetFPS = 20;
            const int frameDelay = 1000 / targetFPS;

            while (running)
            {
                SDL_Event event;
                while (SDL_PollEvent(&event))
                {
                    switch (event.type)
                    {
                        case SDL_EVENT_QUIT:
                            running = false;
                            break;
                        case SDL_EVENT_WINDOW_RESIZED:
                            resized = true;
                            break;
                    }
                }

                uint32_t currentTime = SDL_GetTicks();
                uint32_t frametime = currentTime - lastTime;

                if (frametime >= 1000)
                {
                    float fps = 1000.0f * (float)frameCount / (float)frametime;

                    std::snprintf(titleBuffer, 64, "FPS: %f", fps);
                    SDL_SetWindowTitle(window, titleBuffer);
                    lastTime = currentTime;
                    frameCount = 0u;
                }
                SDL_Delay(frameDelay);
            }

            renderThread.join();
        }

        uint32_t imageIndex;
        uint32_t currentFrame = 0;
        VkSemaphore imgRendered = VK_NULL_HANDLE;

        VkCommandBufferSubmitInfo cmdBufInfo
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO
        };
        VkSemaphoreSubmitInfo waitInfo
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        VkSemaphoreSubmitInfo signalInfo
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
        };
        const VkSubmitInfo2 submitInfo
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount   = 1,
            .pWaitSemaphoreInfos      = &waitInfo,
            .commandBufferInfoCount   = 1,
            .pCommandBufferInfos      = &cmdBufInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos    = &signalInfo,
        };
        const VkPresentInfoKHR presentInfo
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &imgRendered,
            .swapchainCount = 1,
            .pSwapchains = &frameManager.swapChain,
            .pImageIndices = &imageIndex,
            .pResults = nullptr
        };
        void createRenderthread()
        {
            renderThread = std::thread([this]()
            {
                setThreadAffinityAndPriority();

                while (running)
                {
                    const VkFence fence = syncManager.inFlightFences[currentFrame];
                    const VkSemaphore imgAvailable = syncManager.imageAvailableSemaphores[currentFrame];
                    vkWaitForFences(deviceManager.device, 1, &fence, VK_TRUE, UINT64_MAX);

                    VkResult result = vkAcquireNextImageKHR(deviceManager.device, frameManager.swapChain, UINT64_MAX, imgAvailable, VK_NULL_HANDLE, &imageIndex);

                    if (result == VK_ERROR_OUT_OF_DATE_KHR)
                    {
                        recreateSwapChain();
                        continue;
                    }
                    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
                    {
                        throw std::runtime_error("failed to acquire swap chain image!");
                    }

                    vkResetFences(deviceManager.device, 1, &fence);

                    submitQueue(fence, imgAvailable);
                    presentImg();

                    currentFrame = ++currentFrame % MAX_FRAMES_IN_FLIGHT;
                    frameCount++;
                }
            });
        }
        void submitQueue(const VkFence fence, const VkSemaphore imgAvailable)
        {
            imgRendered = syncManager.renderFinishedSemaphores[imageIndex];
            cmdBufInfo.commandBuffer = commandManager.commandBuffers[imageIndex];
            waitInfo.semaphore = imgAvailable;
            signalInfo.semaphore = imgRendered;
            vkQueueSubmit2(deviceManager.graphicsQueue, 1, &submitInfo, fence);
        }
        void presentImg()
        {
            VkResult result = vkQueuePresentKHR(deviceManager.presentQueue, &presentInfo);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || resized.exchange(false))
            {
                recreateSwapChain();
            }
            else if (result != VK_SUCCESS)
            {
                throw std::runtime_error("failed to present swap chain image!");
            }
        }

        void cleanAll()
        {
            vkDeviceWaitIdle(deviceManager.device);
            frameManager.cleanupSwapChain(deviceManager.device);
            frameManager.cleanupPipeline(deviceManager.device);
            syncManager.cleanupSyncObjects(deviceManager.device);

            vkDestroyBuffer(deviceManager.device, indexBuffer, nullptr);
            vkFreeMemory(deviceManager.device, indexBufferMemory, nullptr);

            vkDestroyBuffer(deviceManager.device, vertexBuffer, nullptr);
            vkFreeMemory(deviceManager.device, vertexBufferMemory, nullptr);

            vkDestroyCommandPool(deviceManager.device, commandManager.commandPool, nullptr);
            vkDestroyDevice(deviceManager.device, nullptr);

            cleanInstance();
        }
        void cleanInstance()
        {
            vkDestroySurfaceKHR(deviceManager.instance, deviceManager.surface, nullptr);

            if (enableValidationLayers) debugManager.DestroyDebugUtilsMessengerEXT(deviceManager.instance, nullptr);

            vkDestroyInstance(deviceManager.instance, nullptr);

            SDL_DestroyWindow(window);
            SDL_Quit();
        }
};

int main(int argc, char* argv[])
{
    Triangle app;
    return EXIT_SUCCESS;
}