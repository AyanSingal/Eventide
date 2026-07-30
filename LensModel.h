#pragma once
#define GLFW_INCLUDE_VULKAN

#include "VulkanContext.h"
#include "ResourceManager.h"

class LensModel
{
public:
    void init(VulkanContext& vulkanContext, ResourceManager& resourceManager, const std::string& weightsPath);
    void cleanup();

    VkBuffer weightsBuffer;
    VkBuffer normConstantsBuffer;

private:
    VulkanContext* context = nullptr;
    ResourceManager* resourceManager = nullptr;

    VkDeviceMemory weightsMemory;
    VkDeviceMemory normConstantsMemory;
    void* normConstantsMapped;

    void loadWeights(const std::string& weightsPath);
    void createNormConstants();
};


