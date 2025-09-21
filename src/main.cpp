#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

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
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>

#include <GL/glew.h>

#include <cstdlib>
#include <vector>
#include <cstdint>
#include <atomic>
#include <iostream>
#include <stdexcept>

#include "common.hpp"
#include "vk_debug.hpp"
#include "vk_device.hpp"
#include "vk_frames.hpp"
#include "vk_buffers.hpp"
#include "vk_command.hpp"
#include "vk_sync.hpp"

#include "gl_shader.hpp"
#include "gl_loadGLTF.hpp"

const bool forceOpenGL = false;

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
class VulkanEngine
{
    public:
        bool initVulkan()
        {
            initWindow();
            volkInitialize();

            if (enableValidationLayers) debugManager.init();
            deviceManager.createInstance(window, debugManager.debugCreateInfo);
            if (enableValidationLayers) debugManager.setupDebugMessenger(deviceManager.instance);
            if (!deviceManager.init()) 
            {
                cleanInstance();
                return false;
            }

            bufferManager.init(deviceManager.physicalDevice, deviceManager.device, deviceManager.indices, deviceManager.graphicsQueue, modelPaths);
            frameManager.init(deviceManager.physicalDevice, deviceManager.device, window, deviceManager.surface, deviceManager.indices, deviceManager.graphicsQueue, deviceManager.swapChainSupport, bufferManager.descriptorSetLayouts);

            commandManager.init(deviceManager.device, deviceManager.indices.graphicsFamily.value(), frameManager.swapChainImages.size(), frameManager.swapChainFramebuffers, frameManager.swapChainExtent, frameManager.graphicsPipeline, frameManager.pipelineLayout, frameManager.renderPass, bufferManager.descriptorSets, bufferManager.models);
            syncManager.createSyncObjects(deviceManager.device);

            createRenderthread();
            mainLoop();
            cleanAll();

            return true;
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
            window = SDL_CreateWindow("...", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
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
        }

        VkFence fence;
        VkSemaphore imgAvailable;
        VkSemaphore imgRendered = VK_NULL_HANDLE;
        uint32_t imageIndex;
        uint32_t currentFrame = 0;

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
            renderThread.join();

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
class OpenGLEngine
{
    public:
        void initOpenGL()
        {
            initWindow();
            createRenderthread();
            mainLoop();
            cleanAll();
        }
    private:
        SDL_Window* window;

        void initWindow()
        {
            SDL_Init(SDL_INIT_VIDEO);

            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

            window = SDL_CreateWindow("...", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
        }

        std::atomic<bool> running = true;
        std::atomic<bool> resized = false;
        std::atomic<uint32_t> frameCount = 0;
        std::thread renderThread;
        std::thread windowThread;

        void mainLoop()
        {
            setThreadAffinityAndPriority();

            uint32_t lastTime = SDL_GetTicks();
            char titleBuffer[64];

            const int targetFPS = 60;
            const int frameDelay = 1000 / targetFPS;

            while (running)
            {
                bool updateCam = false;
                glm::vec2 move = glm::vec2(0,0);

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
                        case SDL_EVENT_MOUSE_MOTION:
                            //playobj.object.cam.eulers.y = std::min(0.9, std::max(-0.9, playobj.object.cam.eulers.y - event.motion.yrel * 0.001));
                            //playobj.object.cam.eulers.x -= event.motion.xrel * 0.001;
                            updateCam = true;
                            break;
                        case SDL_EVENT_KEY_DOWN:
                            switch (event.key.scancode)
                            {
                                case SDL_SCANCODE_W: case SDL_SCANCODE_UP:    move.y += 1; break;
                                case SDL_SCANCODE_S: case SDL_SCANCODE_DOWN:  move.y -= 1; break;
                                case SDL_SCANCODE_A: case SDL_SCANCODE_LEFT:  move.x -= 1; break;
                                case SDL_SCANCODE_D: case SDL_SCANCODE_RIGHT: move.x += 1; break;
                                default: break;
                            }
                    }
                }

                //if (move.x or move.y) playobj.object.update(move, deltaTime); playobj.object.updateCam();
                //if (updateCam) playobj.object.updateCam();

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

        GLint modelpos;
        void createRenderthread()
        {
            renderThread = std::thread([this]()
            {
                setThreadAffinityAndPriority();

                SDL_GL_CreateContext(window);

                glewExperimental = GL_TRUE;
                if (glewInit() != GLEW_OK)
                {
                    std::cerr << "Failed to initialize GLEW\n";
                    return;
                }

                SDL_GL_SetSwapInterval(0);
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LEQUAL);
                glEnable(GL_BLEND);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glFrontFace(GL_CCW);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glViewport(0, 0, WIDTH, HEIGHT);
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                SDL_GL_SwapWindow(window);

                unsigned int shaderProgram = makeShader("shaders/openGL/shader3D.vs", "shaders/openGL/shader3D.fs");
                updateUniformBuffer(shaderProgram);

                std::vector<GlModel> models;
                for (const auto& filename : modelPaths)
                {
                    models.emplace_back(filename.c_str());
                }

                while (running)
                {
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glUseProgram(shaderProgram);

                    //glUniformMatrix4fv(modelpos, 1, GL_FALSE, &playobj.object.transmat[0][0]);
                    //playobj.model->drawModel();

                    static auto startTime = std::chrono::high_resolution_clock::now();
                    auto currentTime = std::chrono::high_resolution_clock::now();
                    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

                    //for (GlModel &model : models)
                    for (int i = 0; i < models.size(); i++)
                    {
                        glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                        model[3][2] -= 5;
                        model[3][0] += ((i % 5) - 2.0) * 1.3;
                        model[3][1] += (i/5) * 1.3;
                        glUniformMatrix4fv(modelpos, 1, GL_FALSE, &model[0][0]);
                        models[i].drawModel();
                    }

                    glDisable(GL_CULL_FACE);

                    //glUseProgram(shaderSkybox);
                    //glDrawArrays(GL_TRIANGLES, 0, 36);
                    //drawGrass();
                    //glUseProgram(shaderBoundingbox);
                    //glDrawArrays(GL_TRIANGLES, 0, 36);

                    glEnable(GL_CULL_FACE);

                    SDL_GL_SwapWindow(window);
                    frameCount++;
                }
            });
        }
        void updateUniformBuffer(int shader)
        {
            glm::vec3 position = glm::vec3(0,5.5,3);
            glm::vec3 eulers = glm::vec3(3.1415/2,-3.1415/8,0);

            float cosX = std::cos(eulers.x);
            float sinX = std::sin(eulers.x);
            float cosY = std::cos(eulers.y);
            float sinY = std::sin(eulers.y);

            glm::vec3 forward = glm::vec3(cosX*cosY, sinY, -sinX*cosY);
            glm::vec3 right = glm::vec3(sinX, 0, cosX);
            glm::vec3 up = glm::vec3(-cosX*sinY, cosY, sinX*sinY);

            glm::mat4 view;
            view[0][0] = right.x;
            view[1][0] = right.y;
            view[2][0] = right.z;
            view[3][0] = -glm::dot(right, position);

            view[0][1] = up.x;
            view[1][1] = up.y;
            view[2][1] = up.z;
            view[3][1] = -glm::dot(up, position);

            view[0][2] = -forward.x;
            view[1][2] = -forward.y;
            view[2][2] = -forward.z;
            view[3][2] = glm::dot(forward, position);

            view[0][3] = 0.0f;
            view[1][3] = 0.0f;
            view[2][3] = 0.0f;
            view[3][3] = 1.0f;

            glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float) WIDTH / (float) HEIGHT, 0.1f, 50.0f);

            GLuint uboProjection;
            GLuint uboView;

            glGenBuffers(1, &uboProjection);
            glBindBuffer(GL_UNIFORM_BUFFER, uboProjection);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), &proj, GL_STATIC_DRAW);
            glGenBuffers(1, &uboView);
            glBindBuffer(GL_UNIFORM_BUFFER, uboView);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), &view, GL_DYNAMIC_DRAW);

            GLuint blockIndex;

            blockIndex = glGetUniformBlockIndex(shader, "PROJ");
            glUniformBlockBinding(shader, blockIndex, 0);
            glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex, uboProjection);

            blockIndex = glGetUniformBlockIndex(shader, "VIEW");
            glUniformBlockBinding(shader, blockIndex, 1);
            glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex, uboView);

            modelpos = glGetUniformLocation(shader, "model");
        }
        void cleanAll()
        {
            renderThread.join();
        }
};

int main(int argc, char* argv[])
{
    #ifdef __EMSCRIPTEN__
        pass
    #else
        VulkanEngine vulkanEngine;
        if (forceOpenGL || !vulkanEngine.initVulkan())
        {
            std::cout << "Failed to create vulkan instance\n" << std::endl;

            OpenGLEngine OpenGLEngine;
            OpenGLEngine.initOpenGL();
        }
    #endif

    return EXIT_SUCCESS;
}