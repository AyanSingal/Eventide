#pragma once
#define GLFW_INCLUDE_VULKAN
#include "VulkanContext.h"
#include "CommandManager.h"
#include "ResourceManager.h"
#include "VulkanModel.h"
struct BLAS
{
    VkAccelerationStructureKHR blasHandle;
    VkBuffer blasBuffer;
    VkDeviceMemory blasBufferMemory;
};
struct TLAS
{
    VkAccelerationStructureKHR tlasHandle;
    VkBuffer tlasBuffer;
    VkDeviceMemory tlasBufferMemory;
    VkBuffer instanceBuffer;
    VkDeviceMemory instanceBufferMemory;
};
class RayTracingAS
{
public:
    std::vector<BLAS> BLASes;
    TLAS tlas;
    void init(VulkanContext &context, ResourceManager &resourceManager, CommandManager &commandManager, VulkanModel &model, const std::vector<glm::mat4>& modelMatrices);
    void cleanup();
private:
    VulkanContext* context = nullptr;
    ResourceManager* resourceManager = nullptr;
    CommandManager* commandManager = nullptr;
    std::vector<glm::mat4> modelMatrices;
    void buildBLAS(VulkanModel &model);
    void buildTLAS();
};