#pragma once
#define GLFW_INCLUDE_VULKAN
#include "VulkanContext.h"
#include "ResourceManager.h"
#include "Vertex.h"

class VulkanModel
{
public:
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;

    void init(VulkanContext &context, ResourceManager &resourceManager, const std::string &modelPath);
    size_t getIndicesSize();

    void cleanup();
    
private:
    VulkanContext *context = nullptr;
    ResourceManager *resourceManager = nullptr;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;    
    VkDeviceMemory vertexBufferMemory;
    VkDeviceMemory indexBufferMemory;
    std::string modelPath;

    void load(const std::string &modelPath);
    void createVertexBuffer();
    void createIndexBuffer();
};