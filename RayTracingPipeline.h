#pragma once
#define GLFW_INCLUDE_VULKAN

#include "VulkanContext.h"
#include "ResourceManager.h"
#include "CommandManager.h"
#include "RayTracingAS.h"
#include "VulkanSwapchain.h"
#include "Camera.h"
#include "ShaderUtils.h"


struct RTUniformBufferObject {
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
};

class RayTracingPipeline{
private:
    VulkanContext *context = nullptr;
    ResourceManager *resourceManager = nullptr;
    CommandManager *commandManager = nullptr;
    RayTracingAS *rtAS = nullptr;
    VulkanSwapchain* swapchain = nullptr;
    Camera *camera = nullptr;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    VkImage storageImage;
    VkImageView storageImageView;
    VkDeviceMemory storageImageMemory;

    VkBuffer sbtBuffer;
    VkDeviceMemory sbtMemory;

    VkBuffer uboBuffer;
    VkDeviceMemory uboMemory;
    void* uboMapped;

    void createStorageImage();
    void createDescriptorSets();
    void createPipeline();
    void createShaderBindingTable();
    void createUBO();
    void updateUBO();
};