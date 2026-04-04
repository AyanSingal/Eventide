#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <optional>
#include <vector>
#include <array>
#include <string>
#include <stdexcept>
#include <set>
#include <iostream>

#include "VulkanTypes.h"

class VulkanContext
{
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

public:
    void init(GLFWwindow *window);
    void cleanup();

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                                 VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();

    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkSurfaceKHR surface;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR = nullptr;
    PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR = nullptr;

private:
    void createInstance();
    void createSurface(GLFWwindow *window);

    void pickPhysicalDevice();
    void createLogicalDevice();

    VkDebugUtilsMessengerEXT debugMessenger;
    void setupDebugMessenger();
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);

    bool checkValidationLayerSupport();
    VkSampleCountFlagBits getMaxUsableSampleCount();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    std::vector<const char *> getRequiredExtensions();
};