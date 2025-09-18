#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

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

#include "common.hpp"
#include "vk_debug.hpp"
#include "vk_device.hpp"
#include "vk_frames.hpp"
#include "vk_buffers.hpp"
#include "vk_command.hpp"
#include "vk_sync.hpp"

class Triangle
{
    public:
        Triangle()
        {
            initWindow();

            #ifdef __EMSCRIPTEN__
                pass
            #else
                if (!forceOpenGL && initVulkan())
                {
                    createRenderthread();
                    mainLoop();
                    cleanAll();
                }
                else
                {
                    printf("Failed to create vulkan instance\n");
                    cleanInstance();
                    //init openGL
                }
            #endif
        }

    private:
        SDL_Window* window;

        DebugManager debugManager;
        DeviceManager deviceManager;
        FrameManager frameManager;
        BufferManager bufferManager;
        CommandManager commandManager;
        SyncManager syncManager;

        void initWindow()
        {
            SDL_Init(SDL_INIT_VIDEO);

            uint64_t flags = SDL_WINDOW_RESIZABLE;
            #ifdef __EMSCRIPTEN__
                pass
            #else
                flags |= SDL_WINDOW_VULKAN;
            #endif

            window = SDL_CreateWindow("...", WIDTH, HEIGHT, flags);
        }
        bool initVulkan()
        {
            volkInitialize();

            if (enableValidationLayers) debugManager.init();
            deviceManager.createInstance(window, debugManager.debugCreateInfo);
            if (enableValidationLayers) debugManager.setupDebugMessenger(deviceManager.instance);
            if (!deviceManager.init()) return false;

            std::vector<std::string> modelPaths
            {
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
                "models/vedal987/vedal987.gltf",
            };

            bufferManager.init(deviceManager.physicalDevice, deviceManager.device, deviceManager.indices, deviceManager.graphicsQueue, modelPaths);
            frameManager.init(deviceManager.physicalDevice, deviceManager.device, window, deviceManager.surface, deviceManager.indices, deviceManager.graphicsQueue, deviceManager.swapChainSupport, bufferManager.descriptorSetLayouts);

            commandManager.init(deviceManager.device, deviceManager.indices.graphicsFamily.value(), frameManager.swapChainImages.size(), frameManager.swapChainFramebuffers, frameManager.swapChainExtent, frameManager.graphicsPipeline, frameManager.pipelineLayout, frameManager.renderPass, bufferManager.descriptorSets, bufferManager.models);
            syncManager.createSyncObjects(deviceManager.device);

            return true;
        }
        void recreateSwapChain()
        {
            vkDeviceWaitIdle(deviceManager.device);

            deviceManager.reinit();
            frameManager.reinit(deviceManager.physicalDevice, deviceManager.device, window, deviceManager.surface, deviceManager.indices, deviceManager.graphicsQueue, deviceManager.swapChainSupport);

            vkFreeCommandBuffers(deviceManager.device, commandManager.commandPool, static_cast<uint32_t>(commandManager.commandBuffers.size()), commandManager.commandBuffers.data());
            commandManager.createCommandBuffers(deviceManager.device, frameManager.swapChainImages.size(), frameManager.swapChainFramebuffers, frameManager.swapChainExtent, frameManager.graphicsPipeline, frameManager.pipelineLayout, frameManager.renderPass, bufferManager.descriptorSets, bufferManager.models);
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

        VkFence fence;
        VkSemaphore imgAvailable;
        uint32_t imageIndex;
        uint32_t currentFrame = 0;
        VkSemaphore imgRendered = VK_NULL_HANDLE;

        VkSemaphoreSubmitInfo waitInfo
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        VkCommandBufferSubmitInfo cmdBufInfo
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO
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
                    fence = syncManager.inFlightFences[currentFrame];
                    imgAvailable = syncManager.imageAvailableSemaphores[currentFrame];

                    vkWaitForFences(deviceManager.device, 1, &fence, VK_TRUE, UINT64_MAX);
                    if (!acquireImage()) continue;;
                    vkResetFences(deviceManager.device, 1, &fence);

                    bufferManager.updateUniformBuffer(currentFrame);
                    submitQueue();
                    presentImg();

                    currentFrame = ++currentFrame % MAX_FRAMES_IN_FLIGHT;
                    frameCount++;
                }
            });
        }
        bool acquireImage()
        {
            VkResult result = vkAcquireNextImageKHR(deviceManager.device, frameManager.swapChain, UINT64_MAX, imgAvailable, VK_NULL_HANDLE, &imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                recreateSwapChain();
                return false;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            {
                throw std::runtime_error("failed to acquire swap chain image!");
            }
            return true;
        }
        void submitQueue()
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
            else
            {
                vk_check(result, "failed to present swap chain image!");
            }
        }

        void cleanAll()
        {
            vkDeviceWaitIdle(deviceManager.device);

            syncManager.cleanupSyncObjects(deviceManager.device);
            frameManager.cleanupSwapChain(deviceManager.device);
            frameManager.cleanupPipeline(deviceManager.device);
            bufferManager.destroyAll(deviceManager.device);

            vkDestroyCommandPool(deviceManager.device, commandManager.commandPool, nullptr);
            vkDestroyDevice(deviceManager.device, nullptr);

            cleanInstance();
        }
        void cleanInstance()
        {
            vkDestroySurfaceKHR(deviceManager.instance, deviceManager.surface, nullptr);
            if (enableValidationLayers) debugManager.destroyDebugUtilsMessengerEXT(deviceManager.instance, nullptr);
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