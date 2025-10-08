#include <SDL3/SDL_events.h>
#include <glm/ext/matrix_float4x4.hpp>
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

#include "vulkan/common.hpp"
#include "vulkan/vk_debug.hpp"
#include "vulkan/vk_device.hpp"
#include "vulkan/vk_frames.hpp"
#include "vulkan/vk_objects.hpp"
#include "vulkan/vk_command.hpp"
#include "vulkan/vk_sync.hpp"

#include "openGL/gl_shader.hpp"
#include "openGL/gl_loadGLTF.hpp"

const bool forceOpenGL = false;
const bool EAT_MOUSE = false;

const int targetFPS = 180;
const int frameDelay = 1000 / targetFPS;

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
std::string playerModelFile = "models/vedal987/vedal987.gltf";

class Player
{
    public:
        glm::vec3 camPosition = glm::vec3(0,5.5,3);
        glm::vec3 camEulers = glm::vec3(3.1415/2,-3.1415/8,0);
        glm::vec3 camForward = glm::vec3(0,0,1);
        glm::vec3 camRight = glm::vec3(1,0,0);
        glm::vec3 camUp = glm::vec3(0,1,0);
        float zoom = 5.0;

        std::atomic<float> dot1;
        std::atomic<float> dot2;
        std::atomic<float> dot3;

        glm::vec3 position = glm::vec3(0,0,0);
        glm::vec3 eulers = glm::vec3(0,0,0);
        glm::vec3 forward = glm::vec3(0,0,1);
        glm::vec3 right = glm::vec3(1,0,0);

        glm::mat4 update(glm::vec2 input, float deltaTime)
        {
            float cosX = std::cos(camEulers.x);
            float sinX = std::sin(camEulers.x);
            float cosY = std::cos(camEulers.y);
            float sinY = std::sin(camEulers.y);

            camForward = glm::vec3(cosX*cosY,  sinY, -sinX*cosY);
            camRight   = glm::vec3(sinX,       0,    cosX      );
            camUp      = glm::vec3(-cosX*sinY, cosY, sinX*sinY );

            forward = glm::vec3(cosX,  0, -sinX);
            right   = glm::vec3(sinX, 0, cosX);

            if (input.x || input.y)
            {
                glm::vec3 movement = glm::normalize(forward * input.y + right * input.x);
                //movement = checkCollision(movement) * deltaTime * 0.02f;
                movement = movement * deltaTime * 0.02f;
                position += movement;
            }

            camPosition = position - camForward * zoom;

            glm::mat4 view = glm::mat4(1.0);
            view[0][0] = camRight.x;
            view[1][0] = camRight.y;
            view[2][0] = camRight.z;

            view[0][1] = camUp.x;
            view[1][1] = camUp.y;
            view[2][1] = camUp.z;

            view[0][2] = -camForward.x;
            view[1][2] = -camForward.y;
            view[2][2] = -camForward.z;

            view[0][3] = 0.0f;
            view[1][3] = 0.0f;
            view[2][3] = 0.0f;
            view[3][3] = 1.0f;

            dot1 = -glm::dot(camRight, camPosition);
            dot2 = -glm::dot(camUp, camPosition);
            dot3 = glm::dot(camForward, camPosition);

            return view;
        }
};

class EngineBase
{
    protected:
        SDL_Window* window;
        Player player;

        std::atomic<bool> running = true;
        std::atomic<bool> resized = false;
        std::atomic<bool> updateCam = false;
        std::atomic<uint32_t> frameCount = 0;
        std::atomic<uint32_t> frametime;
        std::atomic<uint32_t> time;

        std::array<bool, SDL_SCANCODE_COUNT> keys{};
        std::atomic<glm::vec2> moveInput;

        constexpr static float skyboxVertices[]
        {
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
        };
        std::array<const char*, 6> skyboxFaces =
        {
            "gfx/skybox/skybox_right.png",
            "gfx/skybox/skybox_left.png",
            "gfx/skybox/skybox_top.png",
            "gfx/skybox/skybox_bottom.png",
            "gfx/skybox/skybox_front.png",
            "gfx/skybox/skybox_back.png"
        };

        uint32_t startTime = SDL_GetTicks();
        uint32_t lastTime = SDL_GetTicks();
        uint32_t lastUpdateTime = SDL_GetTicks();
        char titleBuffer[64];

        void setThreadAffinityAndPriority(const int core_id)
        {
            #ifdef _WIN32
                HANDLE hThread = GetCurrentThread();
                SetThreadAffinityMask(hThread, 1 << 1);
                SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
            #elif __linux__
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(core_id, &cpuset);

                const pthread_t thread = pthread_self();
                pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

                sched_param sch_params;
                sch_params.sched_priority = sched_get_priority_max(SCHED_RR);
                pthread_setschedparam(thread, SCHED_RR, &sch_params);
            #endif
        }
        void pollEvents()
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
                    case SDL_EVENT_MOUSE_MOTION:
                        player.camEulers.y = std::min(0.9, std::max(-0.9, player.camEulers.y - event.motion.yrel * 0.001));
                        player.camEulers.x -= event.motion.xrel * 0.001;
                        updateCam = true;
                        break;
                    case SDL_EVENT_MOUSE_WHEEL:
                        player.zoom -= event.wheel.y;
                        updateCam = true;
                        break;
                    case SDL_EVENT_KEY_DOWN:
                        keys[event.key.scancode] = true;
                        break;
                    case SDL_EVENT_KEY_UP:
                        keys[event.key.scancode] = false;
                        break;
                }
            }
        }
        void inputs()
        {
            glm::vec2 input{0.0, 0.0};
            if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    input.y += 1.0f;
            if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  input.y -= 1.0f;
            if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  input.x -= 1.0f;
            if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) input.x += 1.0f;

            if (updateCam || input.x != 0.0f || input.y != 0.0f)
            {
                moveInput.store(input);
                updateCam.store(true, std::memory_order_release);
            }
        }
        void calculateFramerate()
        {
            uint32_t currentTime = SDL_GetTicks();
            frametime = currentTime - lastTime;
            time = currentTime - startTime;
            lastTime = currentTime;

            uint32_t frameUpdateTime = currentTime - lastUpdateTime;
            if (frameUpdateTime >= 1000)
            {
                float fps = 1000.0f * (float)frameCount / (float)frameUpdateTime;

                std::snprintf(titleBuffer, 64, "FPS: %f", fps);
                SDL_SetWindowTitle(window, titleBuffer);
                lastUpdateTime = currentTime;
                frameCount = 0;
            }
        }
};
class VulkanEngine: EngineBase
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

            objectManager.init(deviceManager.physicalDevice, deviceManager.device, deviceManager.indices, deviceManager.graphicsQueue, modelPaths, playerModelFile);
            frameManager.init(deviceManager.physicalDevice, deviceManager.device, window, deviceManager.surface, deviceManager.indices, deviceManager.graphicsQueue, deviceManager.swapChainSupport, objectManager.descriptorSetLayouts);

            commandManager.init(deviceManager.device, deviceManager.indices.graphicsFamily.value(), frameManager.swapChainImages.size(), frameManager.swapChainFramebuffers, frameManager.swapChainExtent, frameManager.graphicsPipeline, frameManager.pipelineLayout, frameManager.renderPass, objectManager.descriptorSets, objectManager.models, objectManager.player);
            syncManager.createSyncObjects(deviceManager.device);

            createRenderthread();
            mainLoop();
            cleanAll();
            cleanInstance();

            return true;
        }

    private:
        DebugManager debugManager;
        DeviceManager deviceManager;
        FrameManager frameManager;
        ObjectManager objectManager;
        CommandManager commandManager;
        SyncManager syncManager;

        void initWindow()
        {
            SDL_Init(SDL_INIT_VIDEO);
            window = SDL_CreateWindow("...", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

            if (EAT_MOUSE) SDL_SetWindowRelativeMouseMode(window, true);
        }
        void recreateSwapChain()
        {
            vkDeviceWaitIdle(deviceManager.device);

            deviceManager.reinit();
            frameManager.reinit(deviceManager.physicalDevice, deviceManager.device, window, deviceManager.surface, deviceManager.indices, deviceManager.graphicsQueue, deviceManager.swapChainSupport);

            vkFreeCommandBuffers(deviceManager.device, commandManager.commandPool, static_cast<uint32_t>(commandManager.commandBuffers.size()), commandManager.commandBuffers.data());
            commandManager.createCommandBuffers(deviceManager.device, frameManager.swapChainImages.size(), frameManager.swapChainFramebuffers, frameManager.swapChainExtent, frameManager.graphicsPipeline, frameManager.pipelineLayout, frameManager.renderPass, objectManager.descriptorSets, objectManager.models, objectManager.player);
        }

        std::thread renderThread;

        void mainLoop()
        {
            setThreadAffinityAndPriority(0);

            while (running)
            {
                pollEvents();
                inputs();

                for (int i = 0; i < objectManager.models.size(); i++)
                {
                    objectManager.models[i].transmat = glm::rotate(glm::mat4(1.0f), 0.001f * time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    objectManager.models[i].transmat[3][2] -= 5;
                    objectManager.models[i].transmat[3][0] += ((i % 5) - 2.0) * 1.3;
                    objectManager.models[i].transmat[3][1] += int(i/5) * 1.3;
                }

                calculateFramerate();
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
                setThreadAffinityAndPriority(1);
                glm::mat4 cachedViewNoTrans;
                glm::mat4 cachedView;
                int updateView;

                while (running)
                {
                    fence = syncManager.inFlightFences[currentFrame];
                    imgAvailable = syncManager.imageAvailableSemaphores[currentFrame];

                    vkWaitForFences(deviceManager.device, 1, &fence, VK_TRUE, UINT64_MAX);
                    if (!acquireImage()) continue;;
                    vkResetFences(deviceManager.device, 1, &fence);

                    if (updateCam.exchange(false))
                    {
                        cachedViewNoTrans = player.update(moveInput.load(), frametime);
                        cachedView = cachedViewNoTrans;
                        cachedView[3][0] = player.dot1;
                        cachedView[3][1] = player.dot2;
                        cachedView[3][2] = player.dot3;
                        updateView = MAX_FRAMES_IN_FLIGHT;
                    }
                    if (updateView)
                    {
                        objectManager.updateView(currentFrame, cachedView);
                        updateView--;
                    }

                    objectManager.player.transmat = glm::rotate(glm::mat4(1.0f), 0.001f * time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    objectManager.player.transmat[3][0] = player.position.x;
                    objectManager.player.transmat[3][1] = player.position.y;
                    objectManager.player.transmat[3][2] = player.position.z;
                    objectManager.updateUniformBuffer(currentFrame);
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
            objectManager.destroyAll(deviceManager.device);

            vkDestroyCommandPool(deviceManager.device, commandManager.commandPool, nullptr);
            vkDestroyDevice(deviceManager.device, nullptr);
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
class OpenGLEngine: EngineBase
{
    public:
        OpenGLEngine()
        {
            initWindow();
            createRenderthread();
            mainLoop();
        }
        ~OpenGLEngine()
        {
            renderThread.join();
        }

    private:
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

            if (EAT_MOUSE) SDL_SetWindowRelativeMouseMode(window, true);
        }

        std::thread renderThread;

        GLuint projPos;
        GLuint viewPos;
        GLuint viewNoTransPos;
        GLuint modelPos;

        GLuint vaoSkybox;

        GlModel playermodel;
        std::vector<GlModel> models;

        void mainLoop()
        {
            setThreadAffinityAndPriority(0);

            while (running)
            {
                pollEvents();
                inputs();

                //camData.camPos = position;
                //glBindBuffer(GL_UNIFORM_BUFFER, uboCampos);
                //glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camData), &camData);

                for (int i = 0; i < models.size(); i++)
                {
                    models[i].transmat = glm::rotate(glm::mat4(1.0f), 0.001f * time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    models[i].transmat[3][2] -= 5;
                    models[i].transmat[3][0] += ((i % 5) - 2.0) * 1.3;
                    models[i].transmat[3][1] += int(i/5) * 1.3;
                }

                calculateFramerate();
                SDL_Delay(frameDelay);
            }
        }

        void createRenderthread()
        {
            renderThread = std::thread([this]()
            {
                setThreadAffinityAndPriority(1);
                createOpenglContext();

                int shaderProgram = makeShader("shaders/openGL/shader3D.vs", "shaders/openGL/shader3D.fs");
                int shaderSkybox = makeShader("shaders/openGL/shaderSkybox.vs", "shaders/openGL/shaderSkybox.fs");

                int shaders[2] = {shaderProgram, shaderSkybox};
                updateUniformBuffer(shaders);

                glUseProgram(shaderSkybox);
                setSkyboxShader(shaderSkybox);

                glUseProgram(shaderProgram);
                for (const auto& filename : modelPaths)
                {
                    models.emplace_back(filename.c_str());
                }
                playermodel = GlModel(playerModelFile.c_str());

                while (running)
                {
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glUseProgram(shaderProgram);

                    if (updateCam.exchange(false))
                    {
                        glm::mat4 viewmat = player.update(moveInput.load(), frametime);

                        glBindBuffer(GL_UNIFORM_BUFFER, viewNoTransPos);
                        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &viewmat);

                        viewmat[3][0] = player.dot1;
                        viewmat[3][1] = player.dot2;
                        viewmat[3][2] = player.dot3;
                        glBindBuffer(GL_UNIFORM_BUFFER, viewPos);
                        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &viewmat);
                    }

                    playermodel.transmat = glm::rotate(glm::mat4(1.0f), 0.001f * time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    playermodel.transmat[3][0] = player.position.x;
                    playermodel.transmat[3][1] = player.position.y;
                    playermodel.transmat[3][2] = player.position.z;
                    glUniformMatrix4fv(modelPos, 1, GL_FALSE, &playermodel.transmat[0][0]);
                    playermodel.drawModel();

                    for (GlModel &model : models)
                    {
                        glUniformMatrix4fv(modelPos, 1, GL_FALSE, &model.transmat[0][0]);
                        model.drawModel();
                    }

                    glDisable(GL_CULL_FACE);

                    glUseProgram(shaderSkybox);
                    glBindVertexArray(vaoSkybox);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                    //drawGrass();
                    //glUseProgram(shaderBoundingbox);
                    //glDrawArrays(GL_TRIANGLES, 0, 36);

                    glEnable(GL_CULL_FACE);

                    SDL_GL_SwapWindow(window);
                    frameCount++;
                }
            });
        }
        void createOpenglContext()
        {
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
            glEnable(GL_FRAMEBUFFER_SRGB);
            glViewport(0, 0, WIDTH, HEIGHT);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            SDL_GL_SwapWindow(window);
        }
        void updateUniformBuffer(int shaders[])
        {
            glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float) WIDTH / (float) HEIGHT, 0.1f, 50.0f);
            glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 viewNoTrans = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            glGenBuffers(1, &projPos);
            glBindBuffer(GL_UNIFORM_BUFFER, projPos);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), &proj, GL_STATIC_DRAW);
            glGenBuffers(1, &viewPos);
            glBindBuffer(GL_UNIFORM_BUFFER, viewPos);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), &view, GL_DYNAMIC_DRAW);
            glGenBuffers(1, &viewNoTransPos);
            glBindBuffer(GL_UNIFORM_BUFFER, viewNoTransPos);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), &viewNoTrans, GL_DYNAMIC_DRAW);

            GLuint blockIndex;
            for (int i = 0; i < 2; i++)
            {
                int shader = shaders[i];
                blockIndex = glGetUniformBlockIndex(shader, "PROJ");
                glUniformBlockBinding(shader, blockIndex, 0);
                glBindBufferBase(GL_UNIFORM_BUFFER, 0, projPos);
            }

            blockIndex = glGetUniformBlockIndex(shaders[0], "VIEW");
            glUniformBlockBinding(shaders[0], blockIndex, 1);
            glBindBufferBase(GL_UNIFORM_BUFFER, 1, viewPos);

            blockIndex = glGetUniformBlockIndex(shaders[1], "VIEWNOTRANS");
            glUniformBlockBinding(shaders[1], blockIndex, 2);
            glBindBufferBase(GL_UNIFORM_BUFFER, 2, viewNoTransPos);

            modelPos = glGetUniformLocation(shaders[0], "model");
        }
        void setSkyboxShader(int shader)
        {
            glGenVertexArrays(1, &vaoSkybox);
            glBindVertexArray(vaoSkybox);

            GLuint vboSkybox;
            glGenBuffers(1, &vboSkybox);
            glBindBuffer(GL_ARRAY_BUFFER, vboSkybox);
            glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glBindVertexArray(0);

            GLuint tex = makeTex3D(skyboxFaces);
            useTex(tex, GL_TEXTURE0);
        }
        GLuint makeTex3D(const std::array<const char*, 6>& filepath)
        {
            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

            for (unsigned int i = 0; i < 6; ++i)
            {
                SDL_Surface *image = IMG_Load(filepath[i]);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB8_ALPHA8, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
                SDL_DestroySurface(image);
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            return texture;
        }
        void useTex(const GLuint texture, const GLenum unit)
        {
            glActiveTexture(unit);
            glBindTexture(GL_TEXTURE_2D, texture);
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
            OpenGLEngine openglEngine;
        }
    #endif

    return EXIT_SUCCESS;
}