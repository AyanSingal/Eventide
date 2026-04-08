#pragma once
#define GLFW_INCLUDE_VULKAN
#include "VulkanContext.h"
#include "ResourceManager.h"
#include "VulkanTypes.h"

#include <algorithm>

class VulkanSwapchain{
public:
    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;
    VkImage colorImage;
    VkImage depthImage;
    std::vector<VkImage> swapChainImages;
    VkImageView colorImageView;
    std::vector<VkImageView> swapChainImageViews;
    VkImageView depthImageView;
    VkExtent2D swapChainExtent;

    void init(VulkanContext& context, ResourceManager& resourceManager, GLFWwindow* window);
    void recreate();
    void cleanupSwapChain();

private:
    VulkanContext* context = nullptr;
    ResourceManager* resourceManager = nullptr;
    GLFWwindow * window = nullptr;
    VkDeviceMemory depthImageMemory;
    VkDeviceMemory colorImageMemory;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();
    void createColorResources();
    void createDepthResources();
    void createImageViews();
};