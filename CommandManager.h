#pragma once
#define GLFW_INCLUDE_VULKAN
#include "VulkanContext.h"
class CommandManager{
public:
    void init(VulkanContext& context);
    void cleanup();
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool);
    void endSingleTimeCommands(VkCommandBuffer commandBuffer,
                               VkCommandPool commandPool, VkQueue queue);
    
    VkCommandPool graphicsCommandPool;
    VkCommandPool transferCommandPool;
    std::vector<VkCommandBuffer> graphicsCommandBuffers;
private:
    VulkanContext* context = nullptr;
    void createCommandPools();
    void createCommandBuffers();
};