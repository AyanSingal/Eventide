#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>

#include "VulkanContext.h"
#include "CommandManager.h"
#include "ResourceManager.h"
#include "VulkanSwapchain.h"
#include "VulkanTexture.h"
#include "VulkanModel.h"
#include "Renderer.h"
#include "Camera.h"
#include "RayTracingAS.h"
#include "imgui.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "models/FlightHelmet/FlightHelmet.gltf";

class Eventide
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow *window;

    VulkanContext context;
    CommandManager commandManager;
    ResourceManager resourceManager;
    VulkanSwapchain swapchain;
    VulkanModel model;
    Renderer renderer;
    Camera camera;
    RayTracingAS rayTracingAS;

    float lastMouseX = 400.0f; // center of window
    float lastMouseY = 300.0f;
    bool firstMouse = true;


    glm::mat4 baseRotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    std::vector<glm::mat4> modelMatrices = {glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)) * baseRotation, 
                                            glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * baseRotation};

    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetCursorPosCallback(window, mouseCallback);
    }

    void initVulkan()
    {
        context.init(window);
        commandManager.init(context);
        resourceManager.init(context, commandManager);
        swapchain.init(context, resourceManager, window);
        model.init(context, resourceManager, commandManager, MODEL_PATH);
        renderer.init(context, resourceManager, commandManager, swapchain, model, camera, modelMatrices, window);
        rayTracingAS.init(context, resourceManager, commandManager, model, modelMatrices);
    }

    void mainLoop()
    {
        float lastFrame = 0.0f;
        while (!glfwWindowShouldClose(window))
        {
            float currentFrame = glfwGetTime();
            float deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            glfwPollEvents();
            camera.processKeyboard(window, deltaTime);
            renderer.drawFrame();
        }

        vkDeviceWaitIdle(context.device);
    }

    void cleanup()
    {
        rayTracingAS.cleanup();
        swapchain.cleanupSwapChain();
        model.cleanup();
        renderer.cleanup();
        commandManager.cleanup();
        context.cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    static void mouseCallback(GLFWwindow *window, double xPos, double yPos)
    {
        auto app = reinterpret_cast<Eventide *>(glfwGetWindowUserPointer(window));


        if(ImGui::GetIO().WantCaptureMouse){
            app->firstMouse = true;
            return;
        }
        
        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) {
            app->firstMouse = true;
            return;
        }

        if (app->firstMouse)
        {
            app->lastMouseX = xPos;
            app->lastMouseY = yPos;
            app->firstMouse = false;
            return;
        }

        float xOffset = xPos - app->lastMouseX;
        float yOffset = app->lastMouseY - yPos; // reversed: screen Y is top-down

        app->lastMouseX = xPos;
        app->lastMouseY = yPos;

        app->camera.processMouse(xOffset, yOffset);
    }
};

int main()
{
    Eventide app;

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
