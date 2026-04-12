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

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

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
    VulkanTexture texture;
    VulkanModel model;
    Renderer renderer;
    Camera camera;

    float lastMouseX = 400.0f; // center of window
    float lastMouseY = 300.0f;
    bool firstMouse = true;

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
        texture.init(context, resourceManager, commandManager, TEXTURE_PATH);
        model.init(context, resourceManager, MODEL_PATH);
        renderer.init(context, resourceManager, commandManager, swapchain, texture, model, camera);
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
        swapchain.cleanupSwapChain();
        texture.cleanup();
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
