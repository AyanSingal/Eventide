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

    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
    }

    void initVulkan()
    {
        context.init(window);
        commandManager.init(context);
        resourceManager.init(context, commandManager);
        swapchain.init(context, resourceManager, window);
        texture.init(context, resourceManager, commandManager, TEXTURE_PATH);
        model.init(context, resourceManager, MODEL_PATH);
        renderer.init(context, resourceManager, commandManager, swapchain, texture, model);
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
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